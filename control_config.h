#pragma once

// ---------- UI / timing ----------
static const unsigned long SCREEN_INTERVAL_MILLIS = 1000;
static const unsigned long BUTTON_DEBOUNCE_MS = 80;

// ---------- Temperature / demand ----------
static const float HOT_WATER_TARGET_C = 60.0f;
static const float BOILER_LOW_C = 55.0f;
static const float BOILER_MED_C = 65.0f;
static const float BOILER_HIGH_C = 75.0f;
static const float CHIMNEY_DIRTY_THRESHOLD_C = 130.0f;
static const float VALID_TEMP_MIN_C = -40.0f;
static const float VALID_TEMP_MAX_C = 150.0f;

// ---------- Firing stage feeder timing ----------
static const unsigned long FEEDER_ON_LOW_MS = 2000;
static const unsigned long FEEDER_OFF_LOW_MS = 30000;
static const unsigned long FEEDER_ON_MED_MS = 1500;
static const unsigned long FEEDER_OFF_MED_MS = 60000;
static const unsigned long FEEDER_ON_HIGH_MS = 1000;
static const unsigned long FEEDER_OFF_HIGH_MS = 90000;

// ---------- Stall / anti-jam ----------
static const unsigned long STALL_DETECT_MS = 300;
static const unsigned long JAM_PULSE_ON_MS = 500;
static const unsigned long JAM_PULSE_OFF_MS = 700;
static const unsigned long JAM_COOLDOWN_MS = 10000;
static const uint8_t JAM_MAX_PULSES_PER_BLOCK = 6;

// ---------- Flame sensing ----------
static const int FLAME_THRESHOLD = 500;
static const unsigned long FLAME_INTERVAL_MILLIS = 1000;
static const byte FLAME_SENSOR_SAMPLES = 10;
static const unsigned long STARTUP_FLAME_STABLE_MS = 4000;
static const unsigned long STARTUP_FLAME_PROVE_TIMEOUT_MS = 120000;
static const unsigned long RUN_FLAME_LOSS_CONFIRM_MS = 8000;

// ---------- Startup / retry ----------
static const unsigned long STARTUP_PURGE_MS = 15000;
static const unsigned long STARTUP_FEED_MS = 3000;
static const unsigned long STARTUP_RETRY_DELAY_MS = 10000;
static const uint8_t STARTUP_MAX_RETRIES = 2;
static const uint8_t RUN_MAX_RESTARTS = 2;

// ---------- Shutdown ----------
static const unsigned long SHUTDOWN_PUMP_OVERRUN_MS = 90000;
static const unsigned long SHUTDOWN_POST_PURGE_MS = 20000;

// ---------- Fan power profiles ----------
static const int FAN_POWER_MIN = 0;
static const int FAN_POWER_MAX = 100;
static const int FAN_POWER_DEGRADED = 20;
static const int FAN_POWER_RUN_LOW = 80;
static const int FAN_POWER_RUN_MED = 60;
static const int FAN_POWER_RUN_HIGH = 40;
static const int FAN_POWER_STARTUP_PURGE = 100;
static const int FAN_POWER_STARTUP_FEED = 85;
static const int FAN_POWER_STARTUP_PROVE = 70;
static const int FAN_POWER_RESTART_DELAY = 0;
static const int FAN_POWER_SHUTDOWN_OVERRUN = 30;
static const int FAN_POWER_SHUTDOWN_POST_PURGE = 80;
static const int FAN_POWER_IDLE = 0;
