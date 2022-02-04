# Pomace

Building a custom controller board for our family's pomace boiler. 

What the board will need to control:
 - motor to get pomace into the fire (relay, according to light sensor and thermometer) - pin 7
 - fan motor which blows air into the flame (speed controlled, according to light sensor and thermometer)
 - motors bring water from the house (relay, controlled by termometer and requests in the house) - pin 8
 - motor to bring pomace into pyramidal storage (relay, according to a light/fullness sensor to be installed)
 - motor to crunch pomace and bring it to the larger storage (user-actioned, but must add a protection against stalling)
 - heat gun turn on/off (maybe)
 - Will need a clock as well

Sensors the board will have to process:
 - Boiler temperature
 - Temperature of the waters going through the house (sanitary hot water)
 - Temperature inside the house *really a binary value provided by a thermostat inside the house)
 - Waste gas temperature
 - Flame light
 - In the future:
    - Fullness of pyramid storage 
    - Stalling of the motors (especially the pomace breaker)

Components Required:
 - Arduino Mega [BOUGHT]
 - Wifi Module [BOUGHT]
 - 4 Line LCD [BOUGHT]
 - Clock Module [BOUGHT]
 - Relays Shield [BOUGHT]
 - Battery for clock module [BOUGHT]
 - 12V Power Supply for relay module [BOUGHT]
 - I²C Module for Screen [BOUGHT]
 - Male-Female, Male-Male, Female-Female jumper cables [BOUGHT]
 - fio normal de arame [BOUGHT]
 - 4 Additional Temperature sensors [BOUGHT]
 - 4 4.7 Ohm resistors to use with temperatrure sensor [BOUGHT]
 - 10K Ohm, 100K Ohm resistors [BOUGHT]
 - Buttons of some sort [BOUGHT]
 - Dimmer module [BOUGHT]
 - Flame sensor  [BOUGHT, need a more heavy duty one]
 - 6X (at least) Extension cords to use with relays/dimmer (boiler feeder, hot water pump, radiators pump, ashes feeder, fan motor, large feeder) https://www.amazon.es/Brennenstuhl-Alargo-europeo-3-m/dp/B004AQTFWY/ref=sr_1_6?__mk_es_ES=%C3%85M%C3%85%C5%BD%C3%95%C3%91&crid=WNHPZ8I7D70B&keywords=alargadera+enchufe&qid=1641738516&sprefix=alargadera+enchuf%2Caps%2C187&sr=8-6
 - SD Card Module and SD Card

 Currenly missing to get system working:
  - Dimmer working with Fan Control
  - Temperature sensor from hot water
  - Thermostat working (kind of optional)
  - Connections with wires to motors

Interrupts
 - Needed for menus
 - Needed for motor control/dimmer

Web server should be able to:
 - Show the present state of the boiler
 - Provide controls for motors
 - Using Blynk running on our VPS

Data Storage:
 - Timestamp from clock
 - Stored into SD card

 Motor control:
 - I think I can afford to spare one interrupt pin for the dimmer
 - I need to control the speed of an AC single-phase motor
 - 50 Hz, 220 V, 0.09 kW, 2800 RPM


Resources:
 - Connect LCD and rtc clock simmultaneously https://create.arduino.cc/projecthub/Tishin/rtcds3231-with-1602-lcd-i2c-367cb6
 - Connect several relays with a single wire https://create.arduino.cc/projecthub/Pedro52/arduino-with-neopixel-optocouplers-controlling-many-relays-5f2573
 - Connect a thermostat to Arduino https://electronics.stackexchange.com/questions/60857/thermostat-connections-with-arduino
 - Connect the temperature sensors https://lastminuteengineers.com/multiple-ds18b20-arduino-tutorial/
 - How to dim the Fan Motor: https://www.instructables.com/AC-Dimming-and-AC-Motor-Speed-Control-How-to-With-/
 - How to setup the light detector (just a photoresistor): https://create.arduino.cc/projecthub/ccPegasus/photoresistor-brightness-sensor-db3110
 - How to solder I²C module to LCD https://www.youtube.com/watch?v=jTqOqmjpMIQ
 - https://mauser.pt/catalog/
 - https://github.com/marcass/furnace_control
 - https://www.instructables.com/Arduino-Pellet-Stove-Controller/
 - https://www.youtube.com/watch?v=_Zg5DRCHWfk

