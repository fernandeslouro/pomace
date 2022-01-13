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

void checkflame(int flamepin, int flame_min_light, int counts, int delay_btw_counts)
{
  int average_light = 0;
  Serial.println("Checking Ignition... ");

  while (average_light < flame_min_light)
  {
    for (int counts = 0; counts < 20; counts++)
    {
      average_light += analogRead(flamepin);
      delay(delay_btw_counts);
    }
    average_light /= 10;
  }
  flameOn = true;
  //Serial.println("Done");
}


void motor_control(int boiler_temperature, unsigned long *onDuration, unsigned long *offDuration)
{
  if (boiler_temperature < 55)
  {
    *onDuration = 2000;
    *offDuration = 30000;
    dimmer.setPower(30);
  }
  else if (boiler_temperature > 55 && boiler_temperature < 65)
  {
    *onDuration = 1000;
    *offDuration = 60000;
    dimmer.setPower(20);
  }
  else if (boiler_temperature > 65)
  {
    *onDuration = 1000;
    *offDuration = 90000;
    dimmer.setPower(10);
  }
}

void flame_counts()
{
  if (millis() - flameSensorPreviousMillis >= FLAME_INTERVAL_MILLIS)
  {
    flameSensorPreviousMillis = millis();
    flameSum += analogRead(pinFlameSensor);
    flameCounts++;
  }

  if (flameCounts == FLAME_SENSOR_SAMPLES)
  {
    flameOn = flameSum / flameCounts < 500 ? true : false;
    flameCounts = 0;
    flameSum = 0;
  }
}

void loopcount()
{
  loop_counter++;
  if (loop_counter % show_time_counter_every == 0)
  {
    Serial.println(loop_counter);
    time_shown_counter++;
    loop_timer_previous_millis = loop_timer_now;
    loop_timer_now = millis();
    //loop_counter = 0;
    loop_time = (loop_timer_now - loop_timer_previous_millis) / (time_shown_counter * show_time_counter_every);
    Serial.print("Average loop time is ");
    Serial.println(loop_time);
  }
}

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
