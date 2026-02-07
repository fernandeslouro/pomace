#pragma once

// ---------- Hardware profile (locked for panel commissioning) ----------
static const char *HARDWARE_PROFILE = "NPBC_PANEL_V1";

// ---------- Pins (panel mapping) ----------
static const int pinBurnerFeeder = 8;
static const int pinHotWaterPump = 9;
static const int pinCentralHeatingPump = 10;
static const int pinStorageFeeder = 11;
static const int pinFlameSensor = A0;
static const int buttonPin = 13;
static const int dimmerOutputPin = 3;
static const int pinOneWireTempSensors = 7;
static const int pinThermostatDemand = 12;

// Selector switch and safety chain are mandatory in this profile.
static const bool useModeSelectorPins = true;
static const int pinModeAutoSelect = 22;
static const int pinModeRemoteSelect = 23;

static const bool useSafetyInputPins = true;
static const int pinSafetyHighLimit = 24;
static const int pinSafetyBackfire = 25;
static const int pinSafetyEStop = 26;
static const bool safetyInputsActiveLow = true;

// Stall inputs from overload/current-monitor contacts.
static const bool useStallInputs = true;
static const int pinStallSF = 30;
static const int pinStallPH = 31;
static const int pinStallPWH = 32;
static const int pinStallSB = 33;
static const int pinStallFSG = 34;
static const int pinStallFC = 35;
static const int pinStallStorage = 36;
static const int pinStallCrusher = 37;
