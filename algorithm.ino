#include "RTClib.h"
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include "control_config.h"
#include "hardware_profile.h"

// ---------- UI / timing ----------
LiquidCrystal_I2C lcd(0x27, 20, 4);
unsigned long screenPreviousMillis = 0;

// ---------- Control modes ----------
enum ControlMode
{
  MODE_AUTO,
  MODE_OFF,
  MODE_REMOTE
};

enum MotorCommand
{
  CMD_AUTO,
  CMD_FORCE_OFF,
  CMD_FORCE_ON
};

enum MotorId
{
  MOTOR_SF,
  MOTOR_PH,
  MOTOR_PWH,
  MOTOR_SB,
  MOTOR_FSG,
  MOTOR_FC,
  MOTOR_STORAGE,
  MOTOR_CRUSHER,
  MOTOR_COUNT
};

enum SafetyState
{
  SAFETY_NORMAL,
  SAFETY_DEGRADED,
  SAFETY_LOCKOUT
};

enum BoilerPhase
{
  BOILER_IDLE,
  BOILER_STARTUP_PURGE,
  BOILER_STARTUP_FEED,
  BOILER_STARTUP_FLAME_PROVE,
  BOILER_RUN,
  BOILER_RESTART_DELAY,
  BOILER_SHUTDOWN_OVERRUN,
  BOILER_SHUTDOWN_POST_PURGE,
  BOILER_LOCKOUT
};

enum BoilerFault
{
  BOILER_FAULT_NONE,
  BOILER_FAULT_IGNITION_TIMEOUT,
  BOILER_FAULT_FLAME_LOSS,
  BOILER_FAULT_SENSOR_FAULT
};

enum RunStage
{
  RUN_STAGE_1,
  RUN_STAGE_2,
  RUN_STAGE_3
};

struct MotorControl
{
  const char *name;
  int pinOutput;
  int pinStallInput;
  bool outputActiveLow;
  bool stallInputActiveLow;

  MotorCommand remoteCommand;

  bool outputOn;
  bool faultLatched;
  bool recoveryActive;
  bool cooldownActive;
  bool finalAttempt;

  unsigned long outputChangedAtMs;
  unsigned long stallSinceMs;
  unsigned long recoveryStepStartedMs;
  uint8_t pulsesCompleted;
};

struct FanPwmDriver
{
  explicit FanPwmDriver(int outputPin) : pin(outputPin) {}

  void begin()
  {
    pinMode(pin, OUTPUT);
    analogWrite(pin, 0);
  }

  void setPowerPercent(int percent)
  {
    int clamped = percent;
    if (clamped < 0)
    {
      clamped = 0;
    }
    else if (clamped > 100)
    {
      clamped = 100;
    }

    const int duty = (clamped * 255) / 100;
    analogWrite(pin, duty);
  }

  int pin;
};

// ---------- Hardware drivers ----------
FanPwmDriver fanDriver(dimmerOutputPin);
OneWire oneWire(pinOneWireTempSensors);
DallasTemperature sensors(&oneWire);
RTC_DS1307 rtc;

// ---------- Sensors/state ----------
int deviceCount = 0;
float boiler_temp = DEVICE_DISCONNECTED_C;
float hot_water_temp = DEVICE_DISCONNECTED_C;
float waste_gas_temp = DEVICE_DISCONNECTED_C;
bool sensorsHealthy = false;

bool thermostatDemand = true;
bool thermostatOverrideEnabled = false;
bool thermostatOverrideDemand = true;
bool flameOn = false;
bool flameForceOn = false;
unsigned long flameSensorPreviousMillis = 0;
unsigned int flameCounts = 0;
unsigned int flameSum = 0;

int fanPower = FAN_POWER_IDLE;
bool chimney_needs_clean = false;
bool highLimitTripped = false;
bool backfireTripped = false;
bool eStopTripped = false;
bool safetyLockoutLatched = false;
SafetyState safetyState = SAFETY_DEGRADED;

// ---------- Feeder duty cycle ----------
bool feederCycleOn = false;
unsigned long feederCycleChangedMs = 0;
unsigned long feederOnMs = FEEDER_ON_LOW_MS;
unsigned long feederOffMs = FEEDER_OFF_LOW_MS;

// ---------- Boiler process state machine ----------
BoilerPhase boilerPhase = BOILER_IDLE;
BoilerFault boilerFault = BOILER_FAULT_NONE;
bool boilerProcessLockoutLatched = false;
unsigned long boilerPhaseChangedMs = 0;
unsigned long flameProveSinceMs = 0;
unsigned long flameLossSinceMs = 0;
uint8_t ignitionRetries = 0;
uint8_t runRestarts = 0;
bool burnerFeederCommand = false;
bool pumpOverrunCommand = false;
bool runStageOverrideEnabled = false;
RunStage runStageOverride = RUN_STAGE_1;
RunStage activeRunStage = RUN_STAGE_1;

// ---------- Operator controls ----------
ControlMode currentMode = MODE_AUTO;
int buttonState = HIGH;
int lastButtonState = HIGH;
bool buttonPressHandled = false;
unsigned long lastButtonEdgeMs = 0;

// ---------- Motors (non-fan motors for anti-jam) ----------
MotorControl motors[MOTOR_COUNT] = {
    {"SF", pinBurnerFeeder, pinStallSF, true, true, CMD_AUTO, false, false, false, false, false, 0, 0, 0, 0},
    {"PH", pinCentralHeatingPump, pinStallPH, true, true, CMD_AUTO, false, false, false, false, false, 0, 0, 0, 0},
    {"PWH", pinHotWaterPump, pinStallPWH, true, true, CMD_AUTO, false, false, false, false, false, 0, 0, 0, 0},
    {"SB", pinStorageFeeder, pinStallSB, true, true, CMD_AUTO, false, false, false, false, false, 0, 0, 0, 0},
    {"FSG", -1, pinStallFSG, true, true, CMD_AUTO, false, false, false, false, false, 0, 0, 0, 0},
    {"FC", -1, pinStallFC, true, true, CMD_AUTO, false, false, false, false, false, 0, 0, 0, 0},
    {"STORAGE", -1, pinStallStorage, true, true, CMD_AUTO, false, false, false, false, false, 0, 0, 0, 0},
    {"CRUSHER", -1, pinStallCrusher, true, true, CMD_AUTO, false, false, false, false, false, 0, 0, 0, 0}};

// ---------- Forward declarations ----------
void handleSerialInput();
void processCommand(char *line);
int motorIndexByName(const char *name);

void readTemperatureSensors();
bool isValidTemperature(float valueC);

void updateModeSelection();
const char *modeToString(ControlMode mode);
const char *safetyStateToString(SafetyState state);
const char *boilerPhaseToString(BoilerPhase phase);
const char *boilerFaultToString(BoilerFault fault);
const char *runStageToString(RunStage stage);
int countLatchedFaults();
void updateThermostatDemand();
void updateSafetyState();
bool applySafetyGate(int motorIndex, bool desiredRun);

void setBoilerPhase(BoilerPhase phase, unsigned long nowMs);
void setBoilerLockout(BoilerFault fault, unsigned long nowMs);
void clearBoilerLockout();
void updateBoilerControl(unsigned long nowMs);

RunStage resolveRunStage();
void updateFeederDurationsByBoiler();
void updateFeederCycle(unsigned long nowMs);

int clampFanPower(int power);
int computeRunFanPower();
void applyFanPower(int power);
void flame_counts();

bool resolveDesiredMotorState(int motorIndex, bool autoDesired);
bool readStallInput(const MotorControl &motor);
void setMotorOutput(MotorControl &motor, bool on, unsigned long nowMs);
void startRecovery(MotorControl &motor, unsigned long nowMs, bool finalAttempt);
void updateRecovery(MotorControl &motor, bool desiredRun, unsigned long nowMs);
void updateMotor(MotorControl &motor, bool desiredRun, unsigned long nowMs);
void resetMotorFault(MotorControl &motor);

void update_lcd(DateTime now);
void printStatus();

void setup()
{
  Serial.begin(115200);

  fanDriver.begin();

  lcd.init();
  lcd.backlight();

  pinMode(buttonPin, INPUT_PULLUP);
  pinMode(pinThermostatDemand, INPUT_PULLUP);
  if (useModeSelectorPins)
  {
    pinMode(pinModeAutoSelect, INPUT_PULLUP);
    pinMode(pinModeRemoteSelect, INPUT_PULLUP);
  }

  for (int i = 0; i < MOTOR_COUNT; i++)
  {
    if (motors[i].pinOutput >= 0)
    {
      pinMode(motors[i].pinOutput, OUTPUT);
      setMotorOutput(motors[i], false, millis());
    }
    if (useStallInputs && motors[i].pinStallInput >= 0)
    {
      pinMode(motors[i].pinStallInput, INPUT_PULLUP);
    }
  }

  if (useSafetyInputPins)
  {
    pinMode(pinSafetyHighLimit, INPUT_PULLUP);
    pinMode(pinSafetyBackfire, INPUT_PULLUP);
    pinMode(pinSafetyEStop, INPUT_PULLUP);
  }

  sensors.begin();
  deviceCount = sensors.getDeviceCount();

  rtc.begin();

  Serial.println("NPBC-V3M replacement controller V1 boot");
  Serial.print("Hardware profile: ");
  Serial.println(HARDWARE_PROFILE);
  Serial.print("Mode selector pins: ");
  Serial.println(useModeSelectorPins ? "ENABLED" : "DISABLED");
  Serial.print("Safety chain pins: ");
  Serial.println(useSafetyInputPins ? "ENABLED" : "DISABLED");
  Serial.print("Stall inputs: ");
  Serial.println(useStallInputs ? "ENABLED" : "DISABLED");
  Serial.print("OneWire sensors found: ");
  Serial.println(deviceCount);
  printStatus();
}

void loop()
{
  const unsigned long nowMs = millis();

  handleSerialInput();
  updateModeSelection();
  updateThermostatDemand();
  readTemperatureSensors();
  flame_counts();

  chimney_needs_clean = sensorsHealthy && waste_gas_temp > CHIMNEY_DIRTY_THRESHOLD_C;
  updateSafetyState();
  updateBoilerControl(nowMs);

  bool autoDesired[MOTOR_COUNT] = {false};
  autoDesired[MOTOR_SF] = burnerFeederCommand;
  autoDesired[MOTOR_PH] = thermostatDemand || pumpOverrunCommand;
  autoDesired[MOTOR_PWH] = pumpOverrunCommand || (sensorsHealthy ? (hot_water_temp < HOT_WATER_TARGET_C) : false);
  autoDesired[MOTOR_SB] = false;
  autoDesired[MOTOR_FSG] = burnerFeederCommand;
  autoDesired[MOTOR_FC] = false;
  autoDesired[MOTOR_STORAGE] = false;
  autoDesired[MOTOR_CRUSHER] = false;

  for (int i = 0; i < MOTOR_COUNT; i++)
  {
    bool desiredRun = resolveDesiredMotorState(i, autoDesired[i]);
    desiredRun = applySafetyGate(i, desiredRun);
    updateMotor(motors[i], desiredRun, nowMs);
  }

  update_lcd(rtc.now());
}
