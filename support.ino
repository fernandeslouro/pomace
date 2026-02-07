#include <string.h>
#include <ctype.h>
#include "control_config.h"
#include "hardware_profile.h"

static char serialLine[96];
static uint8_t serialLinePos = 0;

bool equalsIgnoreCase(const char *left, const char *right)
{
  if (left == NULL || right == NULL)
  {
    return false;
  }

  while (*left != '\0' && *right != '\0')
  {
    if (tolower((unsigned char)*left) != tolower((unsigned char)*right))
    {
      return false;
    }
    left++;
    right++;
  }

  return *left == '\0' && *right == '\0';
}

bool readActiveInput(int pin, bool activeLow)
{
  bool raw = digitalRead(pin) == HIGH;
  return activeLow ? !raw : raw;
}

bool isValidTemperature(float valueC)
{
  return valueC != DEVICE_DISCONNECTED_C && valueC > VALID_TEMP_MIN_C && valueC < VALID_TEMP_MAX_C;
}

void readTemperatureSensors()
{
  sensors.requestTemperatures();

  boiler_temp = sensors.getTempCByIndex(0);
  hot_water_temp = sensors.getTempCByIndex(1);
  waste_gas_temp = sensors.getTempCByIndex(2);

  sensorsHealthy = isValidTemperature(boiler_temp) && isValidTemperature(hot_water_temp) && isValidTemperature(waste_gas_temp);
}

void updateThermostatDemand()
{
  if (thermostatOverrideEnabled)
  {
    thermostatDemand = thermostatOverrideDemand;
    return;
  }

  // Thermostat DI assumes dry contact to GND when demand is active.
  thermostatDemand = readActiveInput(pinThermostatDemand, true);
}

const char *safetyStateToString(SafetyState state)
{
  if (state == SAFETY_NORMAL)
  {
    return "NORMAL";
  }
  if (state == SAFETY_DEGRADED)
  {
    return "DEGRADED";
  }
  return "LOCKOUT";
}

const char *boilerPhaseToString(BoilerPhase phase)
{
  if (phase == BOILER_IDLE)
  {
    return "IDLE";
  }
  if (phase == BOILER_STARTUP_PURGE)
  {
    return "START_PURGE";
  }
  if (phase == BOILER_STARTUP_FEED)
  {
    return "START_FEED";
  }
  if (phase == BOILER_STARTUP_FLAME_PROVE)
  {
    return "FLAME_PROVE";
  }
  if (phase == BOILER_RUN)
  {
    return "RUN";
  }
  if (phase == BOILER_RESTART_DELAY)
  {
    return "RESTART_WAIT";
  }
  if (phase == BOILER_SHUTDOWN_OVERRUN)
  {
    return "PUMP_OVERRUN";
  }
  if (phase == BOILER_SHUTDOWN_POST_PURGE)
  {
    return "POST_PURGE";
  }
  return "LOCKOUT";
}

const char *boilerFaultToString(BoilerFault fault)
{
  if (fault == BOILER_FAULT_NONE)
  {
    return "NONE";
  }
  if (fault == BOILER_FAULT_IGNITION_TIMEOUT)
  {
    return "IGN_TIMEOUT";
  }
  if (fault == BOILER_FAULT_FLAME_LOSS)
  {
    return "FLAME_LOSS";
  }
  return "SENSOR";
}

const char *runStageToString(RunStage stage)
{
  if (stage == RUN_STAGE_1)
  {
    return "1";
  }
  if (stage == RUN_STAGE_2)
  {
    return "2";
  }
  return "3";
}

void updateSafetyState()
{
  highLimitTripped = false;
  backfireTripped = false;
  eStopTripped = false;

  if (useSafetyInputPins)
  {
    highLimitTripped = readActiveInput(pinSafetyHighLimit, safetyInputsActiveLow);
    backfireTripped = readActiveInput(pinSafetyBackfire, safetyInputsActiveLow);
    eStopTripped = readActiveInput(pinSafetyEStop, safetyInputsActiveLow);
  }

  bool hardTrip = highLimitTripped || backfireTripped || eStopTripped;
  if (hardTrip)
  {
    safetyLockoutLatched = true;
  }

  if (safetyLockoutLatched)
  {
    safetyState = SAFETY_LOCKOUT;
    return;
  }

  if (!sensorsHealthy || chimney_needs_clean)
  {
    safetyState = SAFETY_DEGRADED;
    return;
  }

  safetyState = SAFETY_NORMAL;
}

bool applySafetyGate(int motorIndex, bool desiredRun)
{
  if (safetyState == SAFETY_LOCKOUT)
  {
    return false;
  }

  if (boilerProcessLockoutLatched)
  {
    // In process lockout, keep only circulation pumps available.
    if (motorIndex == MOTOR_PH || motorIndex == MOTOR_PWH)
    {
      return desiredRun;
    }
    return false;
  }

  if (safetyState == SAFETY_DEGRADED)
  {
    // Keep circulation pumps available in degraded mode.
    if (motorIndex == MOTOR_PH || motorIndex == MOTOR_PWH)
    {
      return desiredRun;
    }
    return false;
  }

  return desiredRun;
}

void updateModeSelection()
{
  if (useModeSelectorPins)
  {
    bool autoSelected = digitalRead(pinModeAutoSelect) == LOW;
    bool remoteSelected = digitalRead(pinModeRemoteSelect) == LOW;

    if (remoteSelected)
    {
      currentMode = MODE_REMOTE;
    }
    else if (autoSelected)
    {
      currentMode = MODE_AUTO;
    }
    else
    {
      currentMode = MODE_OFF;
    }
    return;
  }

  buttonState = digitalRead(buttonPin);
  if (buttonState != lastButtonState)
  {
    lastButtonEdgeMs = millis();
    lastButtonState = buttonState;
  }

  if ((millis() - lastButtonEdgeMs) > BUTTON_DEBOUNCE_MS && buttonState == LOW && !buttonPressHandled)
  {
    buttonPressHandled = true;

    if (currentMode == MODE_AUTO)
    {
      currentMode = MODE_OFF;
    }
    else if (currentMode == MODE_OFF)
    {
      currentMode = MODE_REMOTE;
    }
    else
    {
      currentMode = MODE_AUTO;
    }
  }
  else if (buttonState == HIGH)
  {
    buttonPressHandled = false;
  }
}

const char *modeToString(ControlMode mode)
{
  if (mode == MODE_AUTO)
  {
    return "AUTO";
  }
  if (mode == MODE_OFF)
  {
    return "OFF";
  }
  return "REMOTE";
}

int countLatchedFaults()
{
  int faults = 0;
  for (int i = 0; i < MOTOR_COUNT; i++)
  {
    if (motors[i].faultLatched)
    {
      faults++;
    }
  }
  return faults;
}

void updateFeederDurationsByBoiler()
{
  RunStage stage = resolveRunStage();
  activeRunStage = stage;

  if (stage == RUN_STAGE_1)
  {
    feederOnMs = FEEDER_ON_LOW_MS;
    feederOffMs = FEEDER_OFF_LOW_MS;
    return;
  }
  if (stage == RUN_STAGE_2)
  {
    feederOnMs = FEEDER_ON_MED_MS;
    feederOffMs = FEEDER_OFF_MED_MS;
    return;
  }

  feederOnMs = FEEDER_ON_HIGH_MS;
  feederOffMs = FEEDER_OFF_HIGH_MS;
}

void updateFeederCycle(unsigned long nowMs)
{
  unsigned long target = feederCycleOn ? feederOnMs : feederOffMs;
  if (nowMs - feederCycleChangedMs >= target)
  {
    feederCycleOn = !feederCycleOn;
    feederCycleChangedMs = nowMs;
  }
}

RunStage resolveRunStage()
{
  if (runStageOverrideEnabled)
  {
    return runStageOverride;
  }

  if (!sensorsHealthy)
  {
    return RUN_STAGE_3;
  }
  if (boiler_temp <= BOILER_LOW_C)
  {
    return RUN_STAGE_1;
  }
  if (boiler_temp <= BOILER_MED_C)
  {
    return RUN_STAGE_2;
  }
  if (boiler_temp <= BOILER_HIGH_C)
  {
    return RUN_STAGE_3;
  }
  return RUN_STAGE_3;
}

int clampFanPower(int power)
{
  if (power < FAN_POWER_MIN)
  {
    return FAN_POWER_MIN;
  }
  if (power > FAN_POWER_MAX)
  {
    return FAN_POWER_MAX;
  }
  return power;
}

int computeRunFanPower()
{
  RunStage stage = resolveRunStage();
  activeRunStage = stage;

  if (stage == RUN_STAGE_1)
  {
    return FAN_POWER_RUN_LOW;
  }
  if (stage == RUN_STAGE_2)
  {
    return FAN_POWER_RUN_MED;
  }
  if (stage == RUN_STAGE_3)
  {
    return FAN_POWER_RUN_HIGH;
  }

  return FAN_POWER_DEGRADED;
}

void applyFanPower(int power)
{
  fanPower = clampFanPower(power);
  fanDriver.setPowerPercent(fanPower);
}

void setBoilerPhase(BoilerPhase phase, unsigned long nowMs)
{
  if (boilerPhase == phase)
  {
    return;
  }

  boilerPhase = phase;
  boilerPhaseChangedMs = nowMs;

  if (phase != BOILER_STARTUP_FLAME_PROVE)
  {
    flameProveSinceMs = 0;
  }
  if (phase != BOILER_RUN)
  {
    flameLossSinceMs = 0;
  }

  Serial.print("BOILER PHASE ");
  Serial.println(boilerPhaseToString(boilerPhase));
}

void setBoilerLockout(BoilerFault fault, unsigned long nowMs)
{
  boilerProcessLockoutLatched = true;
  boilerFault = fault;
  burnerFeederCommand = false;
  feederCycleOn = false;
  setBoilerPhase(BOILER_LOCKOUT, nowMs);
  applyFanPower(FAN_POWER_MIN);

  Serial.print("BOILER LOCKOUT ");
  Serial.println(boilerFaultToString(boilerFault));
}

void clearBoilerLockout()
{
  boilerProcessLockoutLatched = false;
  boilerFault = BOILER_FAULT_NONE;
  ignitionRetries = 0;
  runRestarts = 0;
  flameProveSinceMs = 0;
  flameLossSinceMs = 0;
}

void updateBoilerControl(unsigned long nowMs)
{
  burnerFeederCommand = false;
  pumpOverrunCommand = false;
  activeRunStage = resolveRunStage();

  bool burnDemand = (currentMode == MODE_AUTO) && thermostatDemand;

  if (safetyState == SAFETY_LOCKOUT)
  {
    setBoilerPhase(BOILER_LOCKOUT, nowMs);
    applyFanPower(FAN_POWER_MIN);
    return;
  }

  switch (boilerPhase)
  {
  case BOILER_IDLE:
    feederCycleOn = false;
    feederCycleChangedMs = nowMs;
    applyFanPower(FAN_POWER_IDLE);

    if (!burnDemand)
    {
      return;
    }

    if (!sensorsHealthy)
    {
      setBoilerLockout(BOILER_FAULT_SENSOR_FAULT, nowMs);
      return;
    }

    if (safetyState == SAFETY_NORMAL && !boilerProcessLockoutLatched)
    {
      ignitionRetries = 0;
      runRestarts = 0;
      setBoilerPhase(BOILER_STARTUP_PURGE, nowMs);
      applyFanPower(FAN_POWER_STARTUP_PURGE);
    }
    return;

  case BOILER_STARTUP_PURGE:
    applyFanPower(FAN_POWER_STARTUP_PURGE);

    if (!burnDemand)
    {
      setBoilerPhase(BOILER_SHUTDOWN_POST_PURGE, nowMs);
      return;
    }
    if (!sensorsHealthy)
    {
      setBoilerLockout(BOILER_FAULT_SENSOR_FAULT, nowMs);
      return;
    }
    if (safetyState != SAFETY_NORMAL || boilerProcessLockoutLatched)
    {
      setBoilerPhase(BOILER_IDLE, nowMs);
      return;
    }
    if (nowMs - boilerPhaseChangedMs >= STARTUP_PURGE_MS)
    {
      setBoilerPhase(BOILER_STARTUP_FEED, nowMs);
    }
    return;

  case BOILER_STARTUP_FEED:
    applyFanPower(FAN_POWER_STARTUP_FEED);
    burnerFeederCommand = true;

    if (!burnDemand)
    {
      setBoilerPhase(BOILER_SHUTDOWN_POST_PURGE, nowMs);
      return;
    }
    if (!sensorsHealthy)
    {
      setBoilerLockout(BOILER_FAULT_SENSOR_FAULT, nowMs);
      return;
    }
    if (safetyState != SAFETY_NORMAL || boilerProcessLockoutLatched)
    {
      setBoilerPhase(BOILER_IDLE, nowMs);
      return;
    }
    if (nowMs - boilerPhaseChangedMs >= STARTUP_FEED_MS)
    {
      burnerFeederCommand = false;
      setBoilerPhase(BOILER_STARTUP_FLAME_PROVE, nowMs);
    }
    return;

  case BOILER_STARTUP_FLAME_PROVE:
    applyFanPower(FAN_POWER_STARTUP_PROVE);

    if (!burnDemand)
    {
      setBoilerPhase(BOILER_SHUTDOWN_POST_PURGE, nowMs);
      return;
    }
    if (!sensorsHealthy)
    {
      setBoilerLockout(BOILER_FAULT_SENSOR_FAULT, nowMs);
      return;
    }
    if (safetyState != SAFETY_NORMAL || boilerProcessLockoutLatched)
    {
      setBoilerPhase(BOILER_IDLE, nowMs);
      return;
    }

    if (flameOn)
    {
      if (flameProveSinceMs == 0)
      {
        flameProveSinceMs = nowMs;
      }
      if (nowMs - flameProveSinceMs >= STARTUP_FLAME_STABLE_MS)
      {
        ignitionRetries = 0;
        runRestarts = 0;
        feederCycleOn = false;
        feederCycleChangedMs = nowMs;
        setBoilerPhase(BOILER_RUN, nowMs);
      }
    }
    else
    {
      flameProveSinceMs = 0;
    }

    if (nowMs - boilerPhaseChangedMs >= STARTUP_FLAME_PROVE_TIMEOUT_MS)
    {
      if (ignitionRetries < STARTUP_MAX_RETRIES)
      {
        ignitionRetries++;
        setBoilerPhase(BOILER_RESTART_DELAY, nowMs);
      }
      else
      {
        setBoilerLockout(BOILER_FAULT_IGNITION_TIMEOUT, nowMs);
      }
    }
    return;

  case BOILER_RUN:
    if (!burnDemand)
    {
      setBoilerPhase(BOILER_SHUTDOWN_OVERRUN, nowMs);
      applyFanPower(FAN_POWER_SHUTDOWN_OVERRUN);
      return;
    }
    if (!sensorsHealthy)
    {
      setBoilerLockout(BOILER_FAULT_SENSOR_FAULT, nowMs);
      return;
    }
    if (safetyState != SAFETY_NORMAL || boilerProcessLockoutLatched)
    {
      setBoilerPhase(BOILER_SHUTDOWN_OVERRUN, nowMs);
      return;
    }

    updateFeederDurationsByBoiler();
    updateFeederCycle(nowMs);
    burnerFeederCommand = feederCycleOn;
    applyFanPower(computeRunFanPower());

    if (flameOn)
    {
      flameLossSinceMs = 0;
      runRestarts = 0;
    }
    else
    {
      if (flameLossSinceMs == 0)
      {
        flameLossSinceMs = nowMs;
      }
      if (nowMs - flameLossSinceMs >= RUN_FLAME_LOSS_CONFIRM_MS)
      {
        if (runRestarts < RUN_MAX_RESTARTS)
        {
          runRestarts++;
          setBoilerPhase(BOILER_RESTART_DELAY, nowMs);
        }
        else
        {
          setBoilerLockout(BOILER_FAULT_FLAME_LOSS, nowMs);
        }
      }
    }
    return;

  case BOILER_RESTART_DELAY:
    feederCycleOn = false;
    applyFanPower(FAN_POWER_RESTART_DELAY);

    if (!burnDemand)
    {
      setBoilerPhase(BOILER_SHUTDOWN_POST_PURGE, nowMs);
      return;
    }
    if (!sensorsHealthy)
    {
      setBoilerLockout(BOILER_FAULT_SENSOR_FAULT, nowMs);
      return;
    }
    if (safetyState != SAFETY_NORMAL || boilerProcessLockoutLatched)
    {
      setBoilerPhase(BOILER_IDLE, nowMs);
      return;
    }
    if (nowMs - boilerPhaseChangedMs >= STARTUP_RETRY_DELAY_MS)
    {
      setBoilerPhase(BOILER_STARTUP_PURGE, nowMs);
    }
    return;

  case BOILER_SHUTDOWN_OVERRUN:
    pumpOverrunCommand = true;
    applyFanPower(FAN_POWER_SHUTDOWN_OVERRUN);

    if (nowMs - boilerPhaseChangedMs >= SHUTDOWN_PUMP_OVERRUN_MS)
    {
      setBoilerPhase(BOILER_SHUTDOWN_POST_PURGE, nowMs);
    }
    return;

  case BOILER_SHUTDOWN_POST_PURGE:
    feederCycleOn = false;
    applyFanPower(FAN_POWER_SHUTDOWN_POST_PURGE);

    if (nowMs - boilerPhaseChangedMs >= SHUTDOWN_POST_PURGE_MS)
    {
      if (burnDemand && safetyState == SAFETY_NORMAL && sensorsHealthy && !boilerProcessLockoutLatched)
      {
        setBoilerPhase(BOILER_STARTUP_PURGE, nowMs);
      }
      else
      {
        setBoilerPhase(BOILER_IDLE, nowMs);
      }
    }
    return;

  case BOILER_LOCKOUT:
    feederCycleOn = false;
    applyFanPower(FAN_POWER_MIN);

    if (boilerProcessLockoutLatched || safetyState == SAFETY_LOCKOUT)
    {
      return;
    }

    if (burnDemand && safetyState == SAFETY_NORMAL && sensorsHealthy)
    {
      setBoilerPhase(BOILER_STARTUP_PURGE, nowMs);
    }
    else
    {
      setBoilerPhase(BOILER_IDLE, nowMs);
    }
    return;
  }
}

void flame_counts()
{
  if (millis() - flameSensorPreviousMillis >= FLAME_INTERVAL_MILLIS)
  {
    flameSensorPreviousMillis = millis();
    flameSum += analogRead(pinFlameSensor);
    flameCounts++;
  }

  if (flameCounts >= FLAME_SENSOR_SAMPLES)
  {
    int average = flameSum / flameCounts;
    flameOn = average < FLAME_THRESHOLD;

    flameCounts = 0;
    flameSum = 0;
  }

  if (flameForceOn)
  {
    flameOn = true;
  }
}

bool resolveDesiredMotorState(int motorIndex, bool autoDesired)
{
  if (boilerProcessLockoutLatched && motorIndex != MOTOR_PH && motorIndex != MOTOR_PWH)
  {
    autoDesired = false;
  }

  if (!sensorsHealthy)
  {
    if (motorIndex != MOTOR_PH && motorIndex != MOTOR_PWH)
    {
      autoDesired = false;
    }
  }

  if (currentMode == MODE_OFF)
  {
    return false;
  }

  if (currentMode == MODE_REMOTE)
  {
    if (motors[motorIndex].remoteCommand == CMD_FORCE_ON)
    {
      return true;
    }
    if (motors[motorIndex].remoteCommand == CMD_FORCE_OFF)
    {
      return false;
    }
  }

  return autoDesired;
}

bool readStallInput(const MotorControl &motor)
{
  if (!useStallInputs || motor.pinStallInput < 0)
  {
    return false;
  }

  return readActiveInput(motor.pinStallInput, motor.stallInputActiveLow);
}

void setMotorOutput(MotorControl &motor, bool on, unsigned long nowMs)
{
  motor.outputOn = on;
  motor.outputChangedAtMs = nowMs;

  if (motor.pinOutput < 0)
  {
    return;
  }

  bool pinLevel = motor.outputActiveLow ? !on : on;
  digitalWrite(motor.pinOutput, pinLevel ? HIGH : LOW);
}

void startRecovery(MotorControl &motor, unsigned long nowMs, bool finalAttempt)
{
  motor.recoveryActive = true;
  motor.cooldownActive = false;
  motor.finalAttempt = finalAttempt;
  motor.pulsesCompleted = 0;
  motor.recoveryStepStartedMs = nowMs;
  setMotorOutput(motor, true, nowMs);
}

void updateRecovery(MotorControl &motor, bool desiredRun, unsigned long nowMs)
{
  if (motor.cooldownActive)
  {
    if (nowMs - motor.recoveryStepStartedMs >= JAM_COOLDOWN_MS)
    {
      startRecovery(motor, nowMs, true);
    }
    return;
  }

  if (!motor.recoveryActive)
  {
    return;
  }

  if (!desiredRun)
  {
    motor.recoveryActive = false;
    motor.cooldownActive = false;
    setMotorOutput(motor, false, nowMs);
    return;
  }

  unsigned long elapsed = nowMs - motor.recoveryStepStartedMs;

  if (motor.outputOn)
  {
    if (elapsed < JAM_PULSE_ON_MS)
    {
      return;
    }

    // If sensor is available and no longer in stall state, recover immediately.
    if (motor.pinStallInput >= 0 && !readStallInput(motor))
    {
      motor.recoveryActive = false;
      motor.cooldownActive = false;
      motor.stallSinceMs = 0;
      setMotorOutput(motor, true, nowMs);
      return;
    }

    setMotorOutput(motor, false, nowMs);
    motor.recoveryStepStartedMs = nowMs;
    return;
  }

  if (elapsed < JAM_PULSE_OFF_MS)
  {
    return;
  }

  motor.pulsesCompleted++;

  if (motor.pulsesCompleted >= JAM_MAX_PULSES_PER_BLOCK)
  {
    if (motor.finalAttempt)
    {
      motor.recoveryActive = false;
      motor.cooldownActive = false;
      motor.faultLatched = true;
      setMotorOutput(motor, false, nowMs);
      return;
    }

    motor.recoveryActive = false;
    motor.cooldownActive = true;
    motor.recoveryStepStartedMs = nowMs;
    setMotorOutput(motor, false, nowMs);
    return;
  }

  setMotorOutput(motor, true, nowMs);
  motor.recoveryStepStartedMs = nowMs;
}

void updateMotor(MotorControl &motor, bool desiredRun, unsigned long nowMs)
{
  if (motor.faultLatched)
  {
    setMotorOutput(motor, false, nowMs);
    return;
  }

  if (motor.recoveryActive || motor.cooldownActive)
  {
    updateRecovery(motor, desiredRun, nowMs);
    return;
  }

  if (!desiredRun)
  {
    motor.stallSinceMs = 0;
    setMotorOutput(motor, false, nowMs);
    return;
  }

  setMotorOutput(motor, true, nowMs);

  if (motor.pinStallInput < 0)
  {
    return;
  }

  bool stallNow = readStallInput(motor);
  if (!stallNow)
  {
    motor.stallSinceMs = 0;
    return;
  }

  if (motor.stallSinceMs == 0)
  {
    motor.stallSinceMs = nowMs;
    return;
  }

  if (nowMs - motor.stallSinceMs >= STALL_DETECT_MS)
  {
    startRecovery(motor, nowMs, false);
  }
}

void resetMotorFault(MotorControl &motor)
{
  motor.faultLatched = false;
  motor.recoveryActive = false;
  motor.cooldownActive = false;
  motor.finalAttempt = false;
  motor.stallSinceMs = 0;
  motor.pulsesCompleted = 0;
}

int motorIndexByName(const char *name)
{
  for (int i = 0; i < MOTOR_COUNT; i++)
  {
    if (equalsIgnoreCase(name, motors[i].name))
    {
      return i;
    }
  }
  return -1;
}

void handleSerialInput()
{
  while (Serial.available() > 0)
  {
    char ch = (char)Serial.read();

    if (ch == '\r')
    {
      continue;
    }

    if (ch == '\n')
    {
      serialLine[serialLinePos] = '\0';
      if (serialLinePos > 0)
      {
        processCommand(serialLine);
      }
      serialLinePos = 0;
      continue;
    }

    if (serialLinePos < sizeof(serialLine) - 1)
    {
      serialLine[serialLinePos++] = ch;
    }
  }
}

void processCommand(char *line)
{
  char *cmd = strtok(line, " ");
  if (cmd == NULL)
  {
    return;
  }

  if (equalsIgnoreCase(cmd, "MODE"))
  {
    char *arg = strtok(NULL, " ");
    if (arg == NULL)
    {
      Serial.println("ERR MODE arg missing");
      return;
    }

    if (equalsIgnoreCase(arg, "AUTO"))
    {
      currentMode = MODE_AUTO;
    }
    else if (equalsIgnoreCase(arg, "OFF"))
    {
      currentMode = MODE_OFF;
    }
    else if (equalsIgnoreCase(arg, "REMOTE"))
    {
      currentMode = MODE_REMOTE;
    }
    else
    {
      Serial.println("ERR MODE invalid");
      return;
    }

    Serial.print("OK MODE ");
    Serial.println(modeToString(currentMode));
    return;
  }

  if (equalsIgnoreCase(cmd, "SET"))
  {
    char *motorName = strtok(NULL, " ");
    char *state = strtok(NULL, " ");

    if (motorName == NULL || state == NULL)
    {
      Serial.println("ERR SET usage: SET <MOTOR> <ON|OFF|AUTO>");
      return;
    }

    int idx = motorIndexByName(motorName);
    if (idx < 0)
    {
      Serial.println("ERR SET motor not found");
      return;
    }

    if (equalsIgnoreCase(state, "ON"))
    {
      motors[idx].remoteCommand = CMD_FORCE_ON;
    }
    else if (equalsIgnoreCase(state, "OFF"))
    {
      motors[idx].remoteCommand = CMD_FORCE_OFF;
    }
    else if (equalsIgnoreCase(state, "AUTO"))
    {
      motors[idx].remoteCommand = CMD_AUTO;
    }
    else
    {
      Serial.println("ERR SET invalid state");
      return;
    }

    Serial.print("OK SET ");
    Serial.print(motors[idx].name);
    Serial.print(" ");
    Serial.println(state);
    return;
  }

  if (equalsIgnoreCase(cmd, "RESET"))
  {
    char *arg = strtok(NULL, " ");
    if (arg == NULL)
    {
      Serial.println("ERR RESET usage: RESET <MOTOR|SAFETY|BOILER|ALL>");
      return;
    }

    if (equalsIgnoreCase(arg, "ALL"))
    {
      for (int i = 0; i < MOTOR_COUNT; i++)
      {
        resetMotorFault(motors[i]);
      }
      safetyLockoutLatched = false;
      clearBoilerLockout();
      setBoilerPhase(BOILER_IDLE, millis());
      Serial.println("OK RESET ALL");
      return;
    }

    if (equalsIgnoreCase(arg, "SAFETY"))
    {
      if (highLimitTripped || backfireTripped || eStopTripped)
      {
        Serial.println("ERR RESET SAFETY blocked by active trip");
        return;
      }
      safetyLockoutLatched = false;
      Serial.println("OK RESET SAFETY");
      return;
    }

    if (equalsIgnoreCase(arg, "BOILER"))
    {
      clearBoilerLockout();
      if (safetyState != SAFETY_LOCKOUT)
      {
        setBoilerPhase(BOILER_IDLE, millis());
      }
      Serial.println("OK RESET BOILER");
      return;
    }

    int idx = motorIndexByName(arg);
    if (idx < 0)
    {
      Serial.println("ERR RESET motor not found");
      return;
    }

    resetMotorFault(motors[idx]);
    Serial.print("OK RESET ");
    Serial.println(motors[idx].name);
    return;
  }

  if (equalsIgnoreCase(cmd, "JAM"))
  {
    char *motorName = strtok(NULL, " ");
    if (motorName == NULL)
    {
      Serial.println("ERR JAM usage: JAM <MOTOR>");
      return;
    }

    int idx = motorIndexByName(motorName);
    if (idx < 0)
    {
      Serial.println("ERR JAM motor not found");
      return;
    }

    if (idx == MOTOR_SF || idx == MOTOR_PH || idx == MOTOR_PWH || idx == MOTOR_SB || idx == MOTOR_FSG || idx == MOTOR_FC || idx == MOTOR_STORAGE || idx == MOTOR_CRUSHER)
    {
      startRecovery(motors[idx], millis(), false);
      Serial.print("OK JAM ");
      Serial.println(motors[idx].name);
      return;
    }

    Serial.println("ERR JAM not supported");
    return;
  }

  if (equalsIgnoreCase(cmd, "THERMOSTAT"))
  {
    char *arg = strtok(NULL, " ");
    if (arg == NULL)
    {
      Serial.println("ERR THERMOSTAT usage: THERMOSTAT <AUTO|0|1>");
      return;
    }

    if (equalsIgnoreCase(arg, "AUTO"))
    {
      thermostatOverrideEnabled = false;
      Serial.println("OK THERMOSTAT AUTO");
      return;
    }

    thermostatOverrideEnabled = true;
    thermostatOverrideDemand = (atoi(arg) != 0);
    thermostatDemand = thermostatOverrideDemand;
    Serial.print("OK THERMOSTAT ");
    Serial.println(thermostatDemand ? "1" : "0");
    return;
  }

  if (equalsIgnoreCase(cmd, "FLAME"))
  {
    char *arg = strtok(NULL, " ");
    if (arg == NULL)
    {
      Serial.println("ERR FLAME usage: FLAME <AUTO|ON>");
      return;
    }

    if (equalsIgnoreCase(arg, "AUTO"))
    {
      flameForceOn = false;
      Serial.println("OK FLAME AUTO");
      return;
    }

    if (equalsIgnoreCase(arg, "ON"))
    {
      flameForceOn = true;
      flameOn = true;
      Serial.println("OK FLAME ON");
      return;
    }

    Serial.println("ERR FLAME invalid");
    return;
  }

  if (equalsIgnoreCase(cmd, "STAGE"))
  {
    char *arg = strtok(NULL, " ");
    if (arg == NULL)
    {
      Serial.println("ERR STAGE usage: STAGE <AUTO|1|2|3>");
      return;
    }

    if (equalsIgnoreCase(arg, "AUTO"))
    {
      runStageOverrideEnabled = false;
      activeRunStage = resolveRunStage();
      Serial.print("OK STAGE AUTO ");
      Serial.println(runStageToString(activeRunStage));
      return;
    }

    if (equalsIgnoreCase(arg, "1"))
    {
      runStageOverride = RUN_STAGE_1;
    }
    else if (equalsIgnoreCase(arg, "2"))
    {
      runStageOverride = RUN_STAGE_2;
    }
    else if (equalsIgnoreCase(arg, "3"))
    {
      runStageOverride = RUN_STAGE_3;
    }
    else
    {
      Serial.println("ERR STAGE invalid");
      return;
    }

    runStageOverrideEnabled = true;
    activeRunStage = runStageOverride;

    // Apply new stage immediately when already running.
    if (boilerPhase == BOILER_RUN)
    {
      feederCycleOn = false;
      feederCycleChangedMs = millis();
    }

    Serial.print("OK STAGE ");
    Serial.println(runStageToString(runStageOverride));
    return;
  }

  if (equalsIgnoreCase(cmd, "STATUS"))
  {
    printStatus();
    return;
  }

  Serial.println("ERR unknown command");
}

void printStatus()
{
  Serial.print("MODE=");
  Serial.print(modeToString(currentMode));
  Serial.print(" SAFETY=");
  Serial.print(safetyStateToString(safetyState));
  Serial.print(" PHASE=");
  Serial.print(boilerPhaseToString(boilerPhase));
  Serial.print(" BFLT=");
  Serial.print(boilerFaultToString(boilerFault));
  Serial.print(" B_LOCK=");
  Serial.print(boilerProcessLockoutLatched ? "1" : "0");
  Serial.print(" RETRY=");
  Serial.print(ignitionRetries);
  Serial.print(" RRESTART=");
  Serial.print(runRestarts);
  Serial.print(" TEMP_OK=");
  Serial.print(sensorsHealthy ? "1" : "0");
  Serial.print(" B=");
  Serial.print(boiler_temp);
  Serial.print(" HW=");
  Serial.print(hot_water_temp);
  Serial.print(" WG=");
  Serial.print(waste_gas_temp);
  Serial.print(" FAN=");
  Serial.print(fanPower);
  Serial.print(" FLAME=");
  Serial.print(flameOn ? "1" : "0");
  Serial.print(" FL_OVR=");
  Serial.print(flameForceOn ? "1" : "0");
  Serial.print(" STG=");
  if (runStageOverrideEnabled)
  {
    Serial.print(runStageToString(runStageOverride));
  }
  else
  {
    Serial.print("AUTO");
  }
  Serial.print(" ASTG=");
  Serial.print(runStageToString(activeRunStage));
  Serial.print(" FAULTS=");
  Serial.println(countLatchedFaults());

  for (int i = 0; i < MOTOR_COUNT; i++)
  {
    Serial.print("MOTOR ");
    Serial.print(motors[i].name);
    Serial.print(" ON=");
    Serial.print(motors[i].outputOn ? "1" : "0");
    Serial.print(" FAULT=");
    Serial.print(motors[i].faultLatched ? "1" : "0");
    Serial.print(" REC=");
    Serial.println(motors[i].recoveryActive ? "1" : "0");
  }
}

void update_lcd(DateTime now)
{
  (void)now;

  if (millis() - screenPreviousMillis < SCREEN_INTERVAL_MILLIS)
  {
    return;
  }

  screenPreviousMillis = millis();

  char phaseTag = '?';
  if (boilerPhase == BOILER_IDLE)
  {
    phaseTag = 'I';
  }
  else if (boilerPhase == BOILER_STARTUP_PURGE)
  {
    phaseTag = 'P';
  }
  else if (boilerPhase == BOILER_STARTUP_FEED)
  {
    phaseTag = 'F';
  }
  else if (boilerPhase == BOILER_STARTUP_FLAME_PROVE)
  {
    phaseTag = 'V';
  }
  else if (boilerPhase == BOILER_RUN)
  {
    phaseTag = 'R';
  }
  else if (boilerPhase == BOILER_RESTART_DELAY)
  {
    phaseTag = 'W';
  }
  else if (boilerPhase == BOILER_SHUTDOWN_OVERRUN)
  {
    phaseTag = 'O';
  }
  else if (boilerPhase == BOILER_SHUTDOWN_POST_PURGE)
  {
    phaseTag = 'G';
  }
  else if (boilerPhase == BOILER_LOCKOUT)
  {
    phaseTag = 'L';
  }

  lcd.setCursor(0, 0);
  lcd.print("M:");
  lcd.print(modeToString(currentMode));
  lcd.print(" S:");
  lcd.print(safetyState == SAFETY_NORMAL ? "N" : (safetyState == SAFETY_DEGRADED ? "D" : "L"));
  lcd.print(" P:");
  lcd.print(phaseTag);
  lcd.print(" B:");
  if (sensorsHealthy)
  {
    lcd.print((int)boiler_temp);
    lcd.print("C");
  }
  else
  {
    lcd.print("ERR");
  }
  lcd.print("   ");

  lcd.setCursor(0, 1);
  lcd.print("HW:");
  if (sensorsHealthy)
  {
    lcd.print((int)hot_water_temp);
    lcd.print("C");
  }
  else
  {
    lcd.print("ERR");
  }
  lcd.print(" F:");
  lcd.print(fanPower);
  lcd.print("% S:");
  lcd.print(runStageToString(activeRunStage));
  lcd.print(runStageOverrideEnabled ? "*" : " ");
  lcd.print(" ");

  lcd.setCursor(0, 2);
  lcd.print("Fl:");
  if (flameOn)
  {
    lcd.print(flameForceOn ? "ON*" : "ON ");
  }
  else
  {
    lcd.print("OFF");
  }
  lcd.print(" SF:");
  lcd.print(motors[MOTOR_SF].outputOn ? "ON " : "OFF");
  lcd.print("R:");
  lcd.print(runRestarts);
  lcd.print(" ");

  lcd.setCursor(0, 3);
  lcd.print("MF:");
  lcd.print(countLatchedFaults());
  lcd.print(" BF:");
  lcd.print(boilerFault == BOILER_FAULT_NONE ? "0" : "1");
  lcd.print(" T:");
  lcd.print(thermostatDemand ? "1" : "0");
  lcd.print(" ");
}
