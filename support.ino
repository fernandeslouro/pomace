void printdate(DateTime now)
{
  Serial.print(now.year(), DEC);
  Serial.print('/');
  Serial.print(now.month(), DEC);
  Serial.print('/');
  Serial.print(now.day(), DEC);
  Serial.print(" ");
  Serial.print(now.hour(), DEC);
  Serial.print(':');
  Serial.print(now.minute(), DEC);
  Serial.print(':');
  Serial.print(now.second(), DEC);
  Serial.println();

  /*

  Serial.print(" (");
  Serial.print(daysOfTheWeek[now.dayOfTheWeek()]);
  Serial.print(") ");

  Serial.print(" since midnight 1/1/1970 = ");
  Serial.print(now.unixtime());
  Serial.print("s = ");
  Serial.print(now.unixtime() / 86400L);
  Serial.println("d");
  */
}

void update_lcd(DateTime now)
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
    lcd.setCursor(7, 1);
    lcd.print("Fan:");
    lcd.print(fanPower);
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

    /*
    lcd.setCursor(15, 3);
    lcd.print(now.hour());
    lcd.print(':');
    lcd.print(now.minute());
    lcd.print(':');
    */
  }
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

void motor_control(int boiler_temperature, unsigned long *onDuration, unsigned long *offDuration, int *fanPower)
{
  if (boiler_temperature < 55)
  {
    *onDuration = 2000;
    *offDuration = 30000;
    *fanPower = 30;
  }
  else if (boiler_temperature > 55 && boiler_temperature < 65)
  {
    *onDuration = 1000;
    *offDuration = 60000;
    *fanPower = 20;
  }
  else if (boiler_temperature > 65)
  {
    *onDuration = 1000;
    *offDuration = 90000;
    *fanPower = 10;
  }
  dimmer.setPower(*fanPower);
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
