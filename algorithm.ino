#include "RTClib.h"
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <OneWire.h>
#include <DallasTemperature.h>

// Set the LCD address to 0x27 for a 16 chars and 2 line display
LiquidCrystal_I2C lcd(0x27, 20, 4);

#define ONE_WIRE_BUS 2               // Data wire is plugged into port 2 on the Arduino
OneWire oneWire(ONE_WIRE_BUS);       // Setup a oneWire instance to communicate with any OneWire devices
DallasTemperature sensors(&oneWire); // Pass our oneWire reference to Dallas Temperature.
int deviceCount = 0;
float tempC, boiler_temp;
bool thermostat = true;

RTC_DS1307 rtc;
char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};

int buttonState = 0;      // variable for reading the pushbutton status
const int buttonPin = 13; // the number of the LED pin

int flameSensorPin = A0;  // select the input pin for LDR
int flameSensorValue = 0; // variable to store the value coming from the sensor

const int pinBurnerFeeder = 3;
const int pinHotWaterPump = 4;
const int pinCentralHeatingPump = 5;
const int pinStorageFeeder = 5;
int loopDelay = 1500;

int motor_running = HIGH; // initial state of LED
long rememberTime = 0;    // this is used by the code

long int onDuration;
long int offDuration;

unsigned long previousMillis = 0;
const int INTERVAL_MILLIS = 1000;
unsigned int sampleCount = 0;
unsigned int total = 0;
const byte SAMPLE_SIZE = 10;

bool chimney_needs_clean;

// Relays to control
// Burner Feeder
// Hot Water pump
// Central Heating Pump
// Storage Feeder
// (Temporary) Ventilator
// (Future) Auxiliary Cleaning Fan
// (Future) Resistor to start flame

// Temperature Sensors
// Boiler Temperature
// Hot Water temperature
// Waste Gas Temperature

void setup()
{
  Serial.begin(9600); // initialize serial monitor with 9600 baud

  pinMode(buttonPin, INPUT);

  sensors.begin(); // Start up the library
  // locate devices on the bus
  Serial.print("Locating devices...");
  Serial.print("Found ");
  deviceCount = sensors.getDeviceCount();
  Serial.print(deviceCount, DEC);
  Serial.println(" devices.");
  Serial.println("");

  /*
  lcd.begin();
  // Turn on the blacklight and print a message.
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Hello, world!");
  */

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

  checkflame(flameSensorPin, true, 800, 10, 1000);
}

void loop()
{
  Serial.println(digitalRead(buttonPin) == HIGH ? "ON" : "OFF");

  // Send command to all the sensors for temperature conversion
  sensors.requestTemperatures();
  Serial.println(flameSensorValue); //prints the values coming from the sensor on the screen

  boiler_temp = sensors.getTempCByIndex(0);

  if (boiler_temp < 55)
  {
    onDuration = 1000;
    offDuration = 30000;
  }
  else if (boiler_temp > 55 && boiler_temp < 65)
  {
    onDuration = 1000;
    offDuration = 60000;
  }
  else if (boiler_temp > 65)
  {
    onDuration = 1000;
    offDuration = 90000;
  }

  if (motor_running == LOW)
  {
    if ((millis() - rememberTime) >= onDuration)
    {
      motor_running = LOW;     // change the state of LED
      rememberTime = millis(); // remember Current millis() time
    }
  }
  else
  {
    if ((millis() - rememberTime) >= offDuration)
    {
      motor_running = HIGH;    // change the state of LED
      rememberTime = millis(); // remember Current millis() time
    }
  }

  digitalWrite(pinBurnerFeeder, motor_running);

  thermostat ? digitalWrite(pinCentralHeatingPump, LOW) : digitalWrite(pinCentralHeatingPump, HIGH);
  sensors.getTempCByIndex(1) < 60 ? digitalWrite(pinHotWaterPump, LOW) : digitalWrite(pinHotWaterPump, HIGH);
  chimney_needs_clean = sensors.getTempCByIndex(2) > 130 ? true : false;

  if (millis() - previousMillis >= INTERVAL_MILLIS)
  {
    previousMillis = millis();
    total = total + analogRead(flameSensorPin);
  }

  if (sampleCount == SAMPLE_SIZE)
  {
    Serial.print(total / sampleCount);

    // reset for the next group
    sampleCount = 0;
    total = 0;
  }
}
