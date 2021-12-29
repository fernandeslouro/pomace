void printdate(DateTime now)
{
    Serial.print(now.year(), DEC);
    Serial.print('/');
    Serial.print(now.month(), DEC);
    Serial.print('/');
    Serial.print(now.day(), DEC);
    Serial.print(" (");
    Serial.print(daysOfTheWeek[now.dayOfTheWeek()]);
    Serial.print(") ");
    Serial.print(now.hour(), DEC);
    Serial.print(':');
    Serial.print(now.minute(), DEC);
    Serial.print(':');
    Serial.print(now.second(), DEC);
    Serial.println();

    /*
  Serial.print(" since midnight 1/1/1970 = ");
  Serial.print(now.unixtime());
  Serial.print("s = ");
  Serial.print(now.unixtime() / 86400L);
  Serial.println("d");
  */
}

void gear(int mainMotorPin, int motor_working_time, int motor_stopped_time)
{
    //dimmer.setPower(present_gear->gear_fan_speed);
    digitalWrite(mainMotorPin, LOW); // turn relay ON
    delay(motor_working_time);
    digitalWrite(mainMotorPin, HIGH); // turn relay OFF
    delay(motor_stopped_time);
}

void checkflame(int flamepin, bool already_running, int flame_min_light, int counts, int delay_btw_counts)
{

    int average_light = 0;
    Serial.println("Checking Ignition... ");

    if (already_running)
    {
        while (average_light < flame_min_light)
        {
            for (int counts = 0; counts < 20; counts++)
            {
                average_light += analogRead(flamepin);
                delay(delay_btw_counts);
            }
            average_light /= 10;
        }
    }
    else
    {
        do
        {
            for (int counts = 0; counts < 10; counts++)
            {
                average_light += analogRead(flameSensorPin);
                delay(delay_btw_counts);
            }
            average_light /= counts;
        } while (average_light > flame_min_light);
    }
    //Serial.println("Done");
}

/*
if (relays)
  {
    for (int i = 0; i < NUMPINS; i++)
    {
      if (relays)
        digitalWrite(controlPin[i], LOW);
      Serial.print("Channel: ");
      Serial.print(i);
      Serial.print(" ON - ");
      printdate(rtc.now());
      delay(loopDelay);
    }
    Serial.println("=====");
    for (int i = 0; i < NUMPINS; i++)
    {
      if (relays)
        digitalWrite(controlPin[i], HIGH);
      Serial.print("Channel: ");
      Serial.print(i);
      Serial.print(" OFF - ");
      printdate(rtc.now());
      delay(loopDelay);
    }
    Serial.println("===============");
  }

  */


/*
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
  */

