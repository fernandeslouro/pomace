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
  /*
  if (digitalRead(buttonPin) == HIGH)
  {
    Serial.println("ON");
  }
  else
  {
    Serial.println("OFF");
  }
*/

  Serial.println(digitalRead(buttonPin) == HIGH ? "ON" : "OFF");

  // Send command to all the sensors for temperature conversion
  sensors.requestTemperatures();
  flameSensorValue = analogRead(flameSensorPin); // read the value from the sensor
  Serial.print("Light sensor value is ");
  Serial.println(flameSensorValue); //prints the values coming from the sensor on the screen

  delay(100);

  // Display temperature from each sensor
  for (int i = 0; i < deviceCount; i++)
  {
    tempC = sensors.getTempCByIndex(i);
    Serial.print("Sensor ");
    Serial.print(i + 1);
    Serial.print(": ");
    Serial.print(tempC);
    Serial.println("C");
    delay(500);
  }
  Serial.println("");

  boiler_temp = sensors.getTempCByIndex(0);

  if (boiler_temp < 55)
  {
    gear(pinBurnerFeeder, 5000, 30000);
  }
  else if (boiler_temp > 55 && boiler_temp < 65)
  {
    gear(pinBurnerFeeder, 5000, 30000);
  }
  else if (boiler_temp > 65)
  {
    gear(pinBurnerFeeder, 5000, 30000);
  }

  if (thermostat)
  {
    digitalWrite(pinCentralHeatingPump, LOW);
  }
  else
  {
    digitalWrite(pinCentralHeatingPump, HIGH);
  }
  
  thermostat ? digitalWrite(pinCentralHeatingPump, LOW) : digitalWrite(pinCentralHeatingPump, HIGH);

  if (sensors.getTempCByIndex(1) < 60)
  {
    digitalWrite(pinHotWaterPump, LOW);
  }
  else
  {
    digitalWrite(pinHotWaterPump, HIGH);
  }

  if (sensors.getTempCByIndex(2) > 130)
  {
    // Display message that chimney must be cleaned
  }

  checkflame(flameSensorPin, true, 800, 5, 200);
}
