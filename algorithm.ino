// add necessary constants
//#define PI 3.14159
 
double temp_measure(int pin){

    const double beta = 3600.0;
    const double r0 = 10000.0;
    const double t0 = 273.0 + 25.0;
    const double rx = r0 * exp(-beta/t0);
 
    // Circuit parameters
    const double vcc = 5.0;
    const double R = 20000.0;
 
    // Number of read ramples
    const int nSamples = 5;
 
    int sum = 0;
    for (int i = 0; i < nSamples; i++) {
        sum += analogRead(pin);
        delay (10);
    }
 
    double v = (vcc*sum)/(nSamples*1024.0);
    double rt = (vcc*R)/v - R;
 
    return beta / log(rt/rx);
}



void setup(){

    const int pinBoilerTemp = A0;
    const int pinHotWaterTemp = A1;
    const int pinHouseTemp = A2;
    const int pinWasteGasTemp = A3;
    const int pinFlameLight = A4;

    // Configures specified pin to work as input ou output
    pinMode(7, OUTPUT);// connected to S terminal of Relay
    pinMode(8, OUTPUT);

    int fan_speed;
    bool fire_motor;
    int average_light = 0;
    int ligt_sensor_value;
    int counts;
    int light_sensor_value;

    // blow the fan at max to clean up a bit
    fan_speed = 100;
    delay(10000);
    fan_speed = 40;

    // check if light is on
    while(average_light < 50){
        for (int counts=0; counts<10; counts++){
            average_light+= analogRead(pinFlameLight);
            delay(1000);
        }
        average_light/=10;
    }

}

class gear_values{
    public:
        int gear_fan_speed;
        int motor_working;
        int motor_stopped;
};

void gear(gear_values *present_gear){

    digitalWrite(7,HIGH);// turn relay ON
    delay(present_gear->motor_working * 1000);
    digitalWrite(7, LOW);// turn relay OFF
    delay(present_gear->motor_stopped * 1000);
}

void loop(){

    //check temperature of boiler every 10 seconds, and chage mode appropriately
    int boiler_temp;
    int boiler_speed;
    int house_temp;
    bool ac_pump;

    gear_values *low;
    gear_values *medium;
    gear_values *high;

        while(true){
            boiler_temp = temp_measure(pinBoilerTemp);

            if (boiler_temp < 55){
                gear(high);
            } else if (boiler_temp > 55 && boiler_temp < 65){
                gear(medium);
            } else if (boiler_temp >65){
                gear(low);
            }

            
            if (temp_measure(pinHouseTemp) < 18){
                digitalWrite(8,HIGH);
            } else{
                digitalWrite(8,LOW);
            }

            if (temp_measure(pinHotWaterTemp) < 60){
                digitalWrite(8,HIGH);
            } else{
                digitalWrite(8,LOW);
            }

            do {
                for (int counts=0; counts<10; counts++){
                    average_light+= light_sensor_value;
                    delay(1000);
                }
                average_light/=10;
            } while(average_light > 50)

        }

    }

}
