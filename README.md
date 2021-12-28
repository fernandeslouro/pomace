# Pomace

Building a custom controller board for our family's pomace boiler. 













 - Temperature insde the house (A2)
 - Waste gas temperature (A3)
 - Flame light (A4)
 - Fullness of pyramid storage 
 - Stalling of the motors (especially the pomace breaker)

Components Required:
 - Arduino Mega https://mauser.pt/catalog/product_info.php?cPath=1667_2889_2891_2888&products_id=096-6804 [BOUGHT]
 - Wifi Module https://mauser.pt/catalog/product_info.php?cPath=1667_2604_2607&products_id=096-6331 [BOUGHT]
 - 4 Line LCD https://mauser.pt/catalog/product_info.php?cPath=1667_2604_2609&products_id=017-1147 [BOUGHT]
 - Clock Module https://mauser.pt/catalog/product_info.php?cPath=1667_2604_2793&products_id=096-6595 [BOUGHT]
 - Relays Shield https://mauser.pt/catalog/product_info.php?cPath=324_517_2118&products_id=096-8204 [BOUGHT]
 - Battery for clock module [BOUGHT]
 - 12V Power Supply for relay module [BOUGHT]
 - I²C Module for Screen https://mauser.pt/catalog/product_info.php?cPath=1667_2604_2609&products_id=096-8614 [BOUGHT]
 - Male-Female, Male-Male, Female-Female jumper cables https://mauser.pt/catalog/product_info.php?cPath=1874_56_2732_2787&products_id=096-7938 [BOUGHT]
 - fio normal de arame [BOUGHT]
 - 4 Additional Temperature sensors https://mauser.pt/catalog/product_info.php?cPath=1667_2669_2676&products_id=096-8756 [BOUGHT]
 - 4 4.7 Ohm resistors to use with temperatrure sensor https://mauser.pt/catalog/product_info.php?cPath=324_526_530&products_id=104-7594 [BOUGHT]
 - 10K Ohm, 100K Ohm resistors [BOUGHT]
 - Buttons of some sort [BOUGHT]
 - Flame sensor  [BOUGHT, need a more heavy duty one]
 - SD Card Module and SD Card
 - Dimmer module



Interrupts
 - Needed for menus
 - Needed for motor control/dimmer

Web server should be able to:
 - Show the present state of the boiler
 - Provide controls for motors

Data Storage:
 - Timestamp from clock
 - Stored into SD card

 Motor control:
 - I think I can afford to spare one interrupt pin for the dimmer
 - I need to control the speed of an AC single-phase motor
 - 50 Hz, 220 V, 0.09 kW, 2800 RPM

Next Steps:
 - Program the entire logic of the boiler control
 - Add a screen and buttons (by this time the boiler can be controlled by the arduino)

Resources:
 - How to connect the temperature sensors https://lastminuteengineers.com/multiple-ds18b20-arduino-tutorial/
 - How to dim the Fan Motor: https://www.instructables.com/AC-Dimming-and-AC-Motor-Speed-Control-How-to-With-/
 - How to setup the light detector (just a photoresistor): https://create.arduino.cc/projecthub/ccPegasus/photoresistor-brightness-sensor-db3110
 - https://mauser.pt/catalog/
 - https://github.com/marcass/furnace_control
 - https://www.instructables.com/Arduino-Pellet-Stove-Controller/
 - https://www.youtube.com/watch?v=_Zg5DRCHWfk

