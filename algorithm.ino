#include "RTClib.h"
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <RBDdimmer.h>

#define outputPin 3
dimmerLamp dimmer(outputPin); //initialase port for dimmer for MEGA, Leonardo, UNO, Arduino M0, Arduino Zero 

// Set the LCD address to 0x27 for a 16 chars and 2 line display
LiquidCrystal_I2C lcd(0x27, 20, 4); // set the LCD address to 0x27 for a 16 chars and 2 line display
int screenPreviousMillis;
const int SCREEN_INTERVAL_MILLIS = 1000;

const int pinBurnerFeeder = 8;
const int pinHotWaterPump = 9;
const int pinCentralHeatingPump = 10;
const int pinStorageFeeder = 11;

const int pinFlameSensor = A0;
const int buttonPin = 13;
const int pinOneWireTempSensors = 7;
bool relays = false;

// Temperature Sensors
OneWire oneWire(pinOneWireTempSensors); // Setup a oneWire instance to communicate with any OneWire devices
DallasTemperature sensors(&oneWire);    // Pass our oneWire reference to Dallas Temperature.
int deviceCount = 0;
float tempC, boiler_temp, hot_water_temp, waste_gas_temp;
bool thermostat = true;

// Real-Time Clock
RTC_DS1307 rtc;
char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};

// Button
int buttonState = 0;

// Flame Sensor
int flameSensorValue = 0; // variable to store the value coming from the sensor
unsigned long flameSensorPreviousMillis = 0;
const int FLAME_INTERVAL_MILLIS = 1000;
unsigned int flameCounts = 0;
unsigned int flameSum = 0;
const byte FLAME_SENSOR_SAMPLES = 10;
bool flameOn;
int motor_running = HIGH; // initial state of LED
long rememberTime = 0;    // this is used by the code

// Other
unsigned long onDuration;
unsigned long offDuration;
bool chimney_needs_clean;

// Count cycle time
int loop_counter;                   //holds the count for every loop pass
int time_shown_counter;             //holds the count for every loop pass
long loop_timer_now;                //holds the current millis
long loop_timer_previous_millis;    //holds the previous millis
float loop_time;                    //holds difference (loop_timer_now - previous_millis) = total execution time
long show_time_counter_every = 100; //Show average time every x loops

void update_lcd()
{
  if (millis() - screenPreviousMillis >= SCREEN_INTERVAL_MILLIS)
  {
    screenPreviousMillis = millis();
    lcd.setCursor(0, 0);
    lcd.print("B:");
    lcd.print(boiler_temp);
    lcd.setCursor(8, 0);
    lcd.print("HW:");
    lcd.print(hot_water_temp);
    lcd.setCursor(0, 1);
    lcd.print("L:");
    lcd.print(analogRead(pinFlameSensor));
    lcd.setCursor(0, 2);
    lcd.print("M:");
    lcd.print(onDuration);
    lcd.print(" ");
    lcd.print(offDuration);
    lcd.print(" ");
    lcd.print(motor_running);
    lcd.setCursor(0, 3);
    lcd.print("T:");
    lcd.print(thermostat);
    lcd.setCursor(4, 3);
    lcd.print("Bt:");
    lcd.print(buttonState);
    lcd.setCursor(9, 3);
    lcd.print("Fl:");
    lcd.print(flameOn);
  }
}

void setup()
{
  Serial.begin(9600);

  dimmer.begin(NORMAL_MODE, ON); //dimmer initialisation: name.begin(MODE, STATE)

  lcd.init(); // initialize the lcd
  lcd.backlight();
  
  pinMode(buttonPin, INPUT);

  sensors.begin(); // Start up the library
  // locate devices on the bus
  Serial.print("Locating devices...");
  Serial.print("Found ");
  deviceCount = sensors.getDeviceCount();
  Serial.print(deviceCount, DEC);
  Serial.println(" devices.");
  Serial.println("");

  Serial.print("Loading Clock... ");
  rtc.begin();
  Serial.println("Done");

  Serial.print("Loading Relays... ");
  pinMode(pinBurnerFeeder, OUTPUT);
  pinMode(pinHotWaterPump, OUTPUT);
  pinMode(pinCentralHeatingPump, OUTPUT);
  pinMode(pinStorageFeeder, OUTPUT);
  digitalWrite(pinBurnerFeeder, HIGH);
  digitalWrite(pinHotWaterPump, HIGH);
  digitalWrite(pinCentralHeatingPump, HIGH);
  digitalWrite(pinStorageFeeder, HIGH);
  Serial.println("Done");

  //checkflame(pinFlameSensor, 800, 3, 1000);
}

void loop()
{
  buttonState = digitalRead(buttonPin);

  sensors.requestTemperatures(); // Send command to all the sensors for temperature conversion

  boiler_temp = sensors.getTempCByIndex(0);
  hot_water_temp = sensors.getTempCByIndex(1);
  waste_gas_temp = sensors.getTempCByIndex(2);

  motor_control(boiler_temp, &onDuration, &offDuration);

  if (motor_running == LOW)
  {
    if ((millis() - rememberTime) >= onDuration)
    {
      motor_running = HIGH;
      rememberTime = millis();
    }
  }
  else
  {
    if ((millis() - rememberTime) >= offDuration)
    {
      motor_running = LOW;
      rememberTime = millis();
    }
  }

  if (relays)
  {
    digitalWrite(pinBurnerFeeder, motor_running);
    thermostat ? digitalWrite(pinCentralHeatingPump, LOW) : digitalWrite(pinCentralHeatingPump, HIGH);
    hot_water_temp < 60 ? digitalWrite(pinHotWaterPump, LOW) : digitalWrite(pinHotWaterPump, HIGH);
  }

  chimney_needs_clean = waste_gas_temp > 130 ? true : false;

  update_lcd();
  flame_counts();
  loopcount();
}