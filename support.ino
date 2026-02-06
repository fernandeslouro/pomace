#include <string.h>
#include <ctype.h>

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
  return valueC != DEVICE_DISCONNECTED_C && valueC > -40.0 && valueC < 150.0;
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
  if (!sensorsHealthy)
  {
    feederOnMs = FEEDER_ON_HIGH_MS;
    feederOffMs = FEEDER_OFF_HIGH_MS;
    return;
  }

  if (boiler_temp <= BOILER_LOW_C)
  {
    feederOnMs = FEEDER_ON_LOW_MS;
    feederOffMs = FEEDER_OFF_LOW_MS;
  }
  else if (boiler_temp <= BOILER_MED_C)
  {
    feederOnMs = FEEDER_ON_MED_MS;
    feederOffMs = FEEDER_OFF_MED_MS;
  }
  else
  {
    feederOnMs = FEEDER_ON_HIGH_MS;
    feederOffMs = FEEDER_OFF_HIGH_MS;
  }
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

void updateFanPowerByBoiler()
{
  if (safetyState == SAFETY_LOCKOUT)
  {
    fanPower = 0;
    dimmer.setPower(fanPower);
    return;
  }

  if (safetyState == SAFETY_DEGRADED)
  {
    fanPower = 20;
    dimmer.setPower(fanPower);
    return;
  }

  if (!sensorsHealthy)
  {
    fanPower = 20;
  }
  else if (boiler_temp < BOILER_LOW_C)
  {
    fanPower = 80;
  }
  else if (boiler_temp < BOILER_MED_C)
  {
    fanPower = 60;
  }
  else if (boiler_temp < BOILER_HIGH_C)
  {
    fanPower = 40;
  }
  else
  {
    fanPower = 20;
  }

  dimmer.setPower(fanPower);
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
}

bool resolveDesiredMotorState(int motorIndex, bool autoDesired)
{
  if (!sensorsHealthy)
  {
    // Keep non-critical outputs off when temperature sensors are invalid.
    if (motorIndex == MOTOR_PH || motorIndex == MOTOR_PWH)
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
      Serial.println("ERR RESET usage: RESET <MOTOR|ALL>");
      return;
    }

    if (equalsIgnoreCase(arg, "ALL"))
    {
      for (int i = 0; i < MOTOR_COUNT; i++)
      {
        resetMotorFault(motors[i]);
      }
      safetyLockoutLatched = false;
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
  if (millis() - screenPreviousMillis < SCREEN_INTERVAL_MILLIS)
  {
    return;
  }

  screenPreviousMillis = millis();

  lcd.setCursor(0, 0);
  lcd.print("M:");
  lcd.print(modeToString(currentMode));
  lcd.print(" S:");
  lcd.print(safetyState == SAFETY_NORMAL ? "N" : (safetyState == SAFETY_DEGRADED ? "D" : "L"));
  lcd.print(" B:");
  if (sensorsHealthy)
  {
    lcd.print((int)boiler_temp);
    lcd.print("C ");
  }
  else
  {
    lcd.print("ERR ");
  }

  lcd.setCursor(0, 1);
  lcd.print("HW:");
  if (sensorsHealthy)
  {
    lcd.print((int)hot_water_temp);
    lcd.print("C ");
  }
  else
  {
    lcd.print("ERR ");
  }
  lcd.print("F:");
  lcd.print(fanPower);
  lcd.print(" ");

  lcd.setCursor(0, 2);
  lcd.print("Fl:");
  lcd.print(flameOn ? "ON " : "OFF");
  lcd.print(" SF:");
  lcd.print(motors[MOTOR_SF].outputOn ? "ON " : "OFF");

  lcd.setCursor(0, 3);
  lcd.print("Faults:");
  lcd.print(countLatchedFaults());
  lcd.print(" T:");
  lcd.print(thermostatDemand ? "1" : "0");
}
