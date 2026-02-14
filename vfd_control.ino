// VFD control layer for fan motor:
// - RUN/STOP on digital output (typically to relay input or VFD DI)
// - Speed command on PWM output (to PWM-to-0..10V converter for VFD AI)

const int pinVfdRun = 3;
const int pinVfdSpeedPwm = 5;

// Change this if your relay module input is active HIGH.
const bool VFD_RUN_PIN_ACTIVE_LOW = true;

const int VFD_PWM_MIN_DUTY = 0;
const int VFD_PWM_MAX_DUTY = 255;

#if defined(ARDUINO_ARCH_ESP32)
const int VFD_PWM_CHANNEL = 0;
const int VFD_PWM_FREQ_HZ = 1000;
const int VFD_PWM_RESOLUTION_BITS = 8;
#endif

int lastFanPowerPercent = -1;
bool lastVfdRunState = false;
bool vfdRunStateInitialized = false;

void vfdSetRun(bool run)
{
  if (vfdRunStateInitialized && run == lastVfdRunState)
  {
    return;
  }

  lastVfdRunState = run;
  vfdRunStateInitialized = true;

  if (VFD_RUN_PIN_ACTIVE_LOW)
  {
    digitalWrite(pinVfdRun, run ? LOW : HIGH);
  }
  else
  {
    digitalWrite(pinVfdRun, run ? HIGH : LOW);
  }
}

void vfdControlBegin()
{
  pinMode(pinVfdRun, OUTPUT);

#if defined(ARDUINO_ARCH_ESP32)
  ledcSetup(VFD_PWM_CHANNEL, VFD_PWM_FREQ_HZ, VFD_PWM_RESOLUTION_BITS);
  ledcAttachPin(pinVfdSpeedPwm, VFD_PWM_CHANNEL);
  ledcWrite(VFD_PWM_CHANNEL, 0);
#else
  pinMode(pinVfdSpeedPwm, OUTPUT);
  analogWrite(pinVfdSpeedPwm, 0);
#endif

  // Safe default at startup.
  vfdSetRun(false);
}

void vfdSetSpeedPercent(int percent)
{
  percent = constrain(percent, 0, 100);

  if (percent == lastFanPowerPercent)
  {
    return;
  }

  lastFanPowerPercent = percent;

  int duty = map(percent, 0, 100, VFD_PWM_MIN_DUTY, VFD_PWM_MAX_DUTY);

#if defined(ARDUINO_ARCH_ESP32)
  ledcWrite(VFD_PWM_CHANNEL, duty);
#else
  analogWrite(pinVfdSpeedPwm, duty);
#endif

  // Optional RUN/STOP driven from speed command.
  vfdSetRun(percent > 0);
}
