#include "RTClib.h"
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <RBDdimmer.h>

// ---------- UI / timing ----------
LiquidCrystal_I2C lcd(0x27, 20, 4);
unsigned long screenPreviousMillis = 0;
const unsigned long SCREEN_INTERVAL_MILLIS = 1000;

// ---------- Pins (current known wiring) ----------
const int pinBurnerFeeder = 8;
const int pinHotWaterPump = 9;
const int pinCentralHeatingPump = 10;
const int pinStorageFeeder = 11;
const int pinFlameSensor = A0;
const int buttonPin = 13;
const int dimmerOutputPin = 3;
const int pinOneWireTempSensors = 7;
const int pinThermostatDemand = 12;

// Optional selector switch inputs (set true once wired)
const bool useModeSelectorPins = false;
const int pinModeAutoSelect = 22;
const int pinModeRemoteSelect = 23;

// Optional safety chain digital inputs (set true once wired)
const bool useSafetyInputPins = false;
const int pinSafetyHighLimit = 24;
const int pinSafetyBackfire = 25;
const int pinSafetyEStop = 26;
const bool safetyInputsActiveLow = true;

// Stall inputs from overload/current-monitor contacts
const bool useStallInputs = true;
const int pinStallSF = 30;
const int pinStallPH = 31;
const int pinStallPWH = 32;
const int pinStallSB = 33;
const int pinStallFSG = 34;
const int pinStallFC = 35;
const int pinStallStorage = 36;
const int pinStallCrusher = 37;

// ---------- Actuator / control constants ----------
const float HOT_WATER_TARGET_C = 60.0;
const float BOILER_LOW_C = 55.0;
const float BOILER_MED_C = 65.0;
const float BOILER_HIGH_C = 75.0;

const unsigned long FEEDER_ON_LOW_MS = 2000;
const unsigned long FEEDER_OFF_LOW_MS = 30000;
const unsigned long FEEDER_ON_MED_MS = 1500;
const unsigned long FEEDER_OFF_MED_MS = 60000;
const unsigned long FEEDER_ON_HIGH_MS = 1000;
const unsigned long FEEDER_OFF_HIGH_MS = 90000;

const unsigned long STALL_DETECT_MS = 300;
const unsigned long JAM_PULSE_ON_MS = 500;
const unsigned long JAM_PULSE_OFF_MS = 700;
const unsigned long JAM_COOLDOWN_MS = 10000;
const uint8_t JAM_MAX_PULSES_PER_BLOCK = 6;

const int FLAME_THRESHOLD = 500;
const unsigned long FLAME_INTERVAL_MILLIS = 1000;
const byte FLAME_SENSOR_SAMPLES = 10;

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

// ---------- Hardware drivers ----------
dimmerLamp dimmer(dimmerOutputPin);
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
unsigned long flameSensorPreviousMillis = 0;
unsigned int flameCounts = 0;
unsigned int flameSum = 0;

int fanPower = 20;
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

// ---------- Operator controls ----------
ControlMode currentMode = MODE_AUTO;
int buttonState = HIGH;
int lastButtonState = HIGH;
bool buttonPressHandled = false;
unsigned long lastButtonEdgeMs = 0;
const unsigned long BUTTON_DEBOUNCE_MS = 80;

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
int countLatchedFaults();
void updateThermostatDemand();
void updateSafetyState();
bool applySafetyGate(int motorIndex, bool desiredRun);

void updateFeederDurationsByBoiler();
void updateFeederCycle(unsigned long nowMs);

void updateFanPowerByBoiler();
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

  dimmer.begin(NORMAL_MODE, ON);

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

  chimney_needs_clean = sensorsHealthy && waste_gas_temp > 130.0;
  updateSafetyState();

  updateFeederDurationsByBoiler();
  updateFeederCycle(nowMs);
  updateFanPowerByBoiler();

  bool autoDesired[MOTOR_COUNT] = {false};
  autoDesired[MOTOR_SF] = feederCycleOn;
  autoDesired[MOTOR_PH] = thermostatDemand;
  autoDesired[MOTOR_PWH] = sensorsHealthy ? (hot_water_temp < HOT_WATER_TARGET_C) : false;
  autoDesired[MOTOR_SB] = false;
  autoDesired[MOTOR_FSG] = feederCycleOn;
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
