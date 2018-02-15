<img src="https://github.com/EKMallon/The_Cave_Pearl_Project_CURRENT_codebuilds/blob/master/images/CavePearlProjectBanner_130x850px.jpg">

This repository contains a basic data logger script that will run on **BOTH** the Pro-Mini based "Modules &amp; Jumper Wires" logger described in the Sensors paper: http://www.mdpi.com/1424-8220/18/2/530 **AND** the UNO based logger described in the **Arduino UNO Datalogger for Complete Beginners** post at:
https://thecavepearlproject.org/2015/12/22/arduino-uno-based-data-logger-with-no-soldering/

<img src="https://github.com/EKMallon/The_Cave_Pearl_Project_CURRENT_codebuilds/blob/master/images/UNObreadboard_600pix.jpg">


In general you only have to do four things to add a new sensor to this logger base code:

1) Download an #include the library that drives your sensor. This is usually provided by the sensor vendor (Adafruit, Sparkfun, etc) 
2) Connect your sensor as appropriate. The easiest ones to work with are often I2C sensors, which should be connected in parallel with the RTC module (since is also an I2C device)
3) Add commands to take a reading from that sensor and put it into a variable at the beginning of the main loop. This is usually means adding:  YourSensorReadingVariable=readsensor();  with the functions provided by the library
4) In the middle of the code where the data is concatenated into the dataString add:

**dataString += ", ";** //comma separates new data from that already in the string

**dataString = dataString + String(YourSensorReadingVariable);**

The code then saves all the ascii characters in dataString to the SD card automatically. Saving data to the SD card at every cycle uses less than 600mAs/day, while a logger that sleeps at 0.25 mA uses ~21,000 mAs during sleep. So you should still see at least 80% of the logger operating lifespan you'd get from the more complicated code. (ie:> 8 months on a full set of AA batteries)

You will find an introduction to the different types of sensors that you can use with the UNO logger at:

**Arduino Tutorial: Adding Sensors to Your Data Logger**
https://thecavepearlproject.org/2017/12/17/adding-sensors-to-an-arduino-data-logger/

If you are using the UNO, check that the sensor you want to use can handle the UNO's 5V positive rail.
