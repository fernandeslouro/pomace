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
 - Water temperature (AO)
 - Temperature of the waters going through the house (sanitary hot water) (A1)
 - Temperature insde the house (A2)
 - Waste gas temperature (A3)
 - Flame light (A4)
 - Fullness of pyramid storage 
 - Stalling of the motors (especially the pomace breaker)

Material needed:
 - Arduino board
 - Screen
 - A bunch of relays

Web server should be able to:
 - Show the present state of the boiler