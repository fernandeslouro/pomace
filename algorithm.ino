/*
#include <RBDdimmer.h>*/
#include <Arduino.h>
#include <math.h>
#include <SD.h>
#include <SPI.h>

// add necessary constants
//#define PI 3.14159

class gear_values
{
public:
    GEAR_VALUES(int gear_fan_speed, int motor_working, int motor_stopped);
    int gear_fan_speed;
    int motor_working;
    int motor_stopped;
};

void gear(gear_values *present_gear)
{
    dimmer.setPower(present_gear->gear_fan_speed);
    digitalWrite(mainMotorPin, HIGH); // turn relay ON
    delay(present_gear->motor_working * 1000);
    digitalWrite(mainMotorPin, LOW); // turn relay OFF
    delay(present_gear->motor_stopped * 1000);
}

double temp_measure(int pin)
{

    const double beta = 3600.0;
    const double r0 = 10000.0;
    const double t0 = 273.0 + 25.0;
    const double rx = r0 * exp(-beta / t0);

    // Circuit parameters
    const double vcc = 5.0;
    const double R = 20000.0;

    // Number of read ramples
    const int nSamples = 5;

    int sum = 0;
    for (int i = 0; i < nSamples; i++)
    {
        sum += analogRead(pin);
        delay(10);
    }

    double v = (vcc * sum) / (nSamples * 1024.0);
    double rt = (vcc * R) / v - R;

    return beta / log(rt / rx);
}

const int pinBoilerTemp = A0;
const int pinHotWaterTemp = A1;
const int pinHouseTemp = A2;
const int pinWasteGasTemp = A3;
const int pinFlameLight = A4;

const int pinCS = 53; //SPI bus on arduino Mega, it's pin 10 on arduino uno
const int mainMotorPin = 7;
const int HouseMotorPin = 8;

const string log_filename = "burner_data.csv"

    low = GEAR_VALUES(50, 1, 90);
medium = GEAR_VALUES(60, 1.5, 70);
high = GEAR_VALUES(70, 2, 60);

void setup()
{
    USE_SERIAL.begin(9600);

    // SD Card Initialization
    if (SD.begin())
    {
        Serial.println("SD card is ready to use.");
    }
    else
    {
        Serial.println("SD card initialization failed");
        return;
    }

    // Configures specified pin to work as input ou output
    pinMode(mainMotorPin, OUTPUT); // connected to S terminal of Relay
    pinMode(HouseMotorPin, OUTPUT);
    pinMode(pinCS, OUTPUT);

    int fan_speed;
    bool fire_motor;
    int average_light = 0;
    int ligt_sensor_value;
    int counts;
    int light_sensor_value;

    dimmer.begin(NORMAL_MODE, ON); //dimmer initialisation: name.begin(MODE, STATE)

    // blow the fan at max to clean up a bit
    dimmer.setPower(100);
    delay(10000);
    dimmer.setPower(40);

    // check if light is on
    while (average_light < 50)
    {
        for (int counts = 0; counts < 10; counts++)
        {
            average_light += analogRead(pinFlameLight);
            delay(1000);
        }
        average_light /= 10;
    }
}

void loop()
{

    //check temperature of boiler every 10 seconds, and chage mode appropriately
    int boiler_temp;
    int boiler_speed;
    int house_temp;
    bool ac_pump;

    /*
    gear_values *low;
    gear_values *medium;
    gear_values *high;
    */

    while (true)
    {
        boiler_temp = temp_measure(pinBoilerTemp);

        if (boiler_temp < 55)
        {
            gear(high);
        }
        else if (boiler_temp > 55 && boiler_temp < 65)
        {
            gear(medium);
        }
        else if (boiler_temp > 65)
        {
            gear(low);
        }

        if (temp_measure(pinHouseTemp) < 18)
        {
            digitalWrite(HouseMotorPin, HIGH);
        }
        else
        {
            digitalWrite(HouseMotorPin, LOW);
        }

        if (temp_measure(pinHotWaterTemp) < 60)
        {
            digitalWrite(HouseMotorPin, HIGH);
        }
        else
        {
            digitalWrite(HouseMotorPin, LOW);
        }

        if (temp_measure(pinWasteGasTemp) > 130)
        {
            // Display message that chimney must be cleaned
        }

        /*
        Evary minute or something like that, store data:
         - boiler temperature
         - waste gas temperature
         - House temperature
         - Hot water temperature
         - Boiler temperature
         - Light sensor?
         - 
        */

        do
        {
            for (int counts = 0; counts < 10; counts++)
            {
                average_light += analogRead(pinFlameLight);
                delay(1000);
            }
            average_light /= 10;
        } while (average_light > 50);
    }

    myFile = SD.open(log_filename, FILE_WRITE);
    if (myFile)
    {
        //myFile.print(rtc.getTimeStr()); (time)
        myfile.print(",");
        myfile.println(int(boiler_temp));
        myfile.print(",");
        myfile.println(int(house_temp));

        myFile.close();
    }
    else
    {
        Serial.println("error opening file");
    }
}
