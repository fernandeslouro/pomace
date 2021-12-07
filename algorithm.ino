

void setup(){

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
            average_light+= light_sensor_value;
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

void gear(bool fire_motor, gear_values *present_gear){
    fire_motor = true;
    delay(present_gear->motor_working * 1000);
    fire_motor = false;
    delay(present_gear->motor_stopped * 1000);
}

void loop(){

    //check temperature of boiler every 10 seconds, and chage mode appropriately
    int boiler_temp;
    int boiler_speed;

    gear_values *low;
    gear_values *medium;
    gear_values *high;

    while(true){
        if (boiler_temp < 55){
            gear(fire_motor, high);
        } else if (boiler_temp > 55 && boiler_temp < 65){
            gear(fire_motor, medium);
        } else if (boiler_temp >65){
            gear(fire_motor, low);
        }
    }

}
