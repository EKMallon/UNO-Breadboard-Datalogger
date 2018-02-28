<img src="https://github.com/EKMallon/The_Cave_Pearl_Project_CURRENT_codebuilds/blob/master/images/CavePearlProjectBanner_130x850px.jpg">

This repository contains a basic data logger script that will run on **BOTH** the Pro-Mini based "Modules &amp; Jumper Wires" logger described in the Sensors paper: http://www.mdpi.com/1424-8220/18/2/530 **AND** the UNO based logger described in the **Arduino UNO Datalogger for Complete Beginners** post at:
https://thecavepearlproject.org/2015/12/22/arduino-uno-based-data-logger-with-no-soldering/

<img src="https://github.com/EKMallon/The_Cave_Pearl_Project_CURRENT_codebuilds/blob/master/images/UNObreadboard_600pix.jpg">


In general you only have to do four things to add a new sensor to this logger base code:

1) Download an #include the library that drives your sensor. This is usually provided by the sensor vendor (Adafruit, Sparkfun, etc) 
2) Connect your sensor as appropriate. The easiest ones to work with are often I2C sensors, which should be connected in parallel with the RTC module (since is also an I2C device)
3) Add commands to take a reading from that sensor and put it into a variable at the beginning of the main loop. This is usually means adding:  YourSensorVariable=readsensor();  with the functions provided by the library
4) In the middle of the code where the data is concatenated into the dataString add:

**dataString = dataString + ",";** //comma separates new data from that already in the string

**dataString = dataString + String(YourSensorVariable);**

The code then saves all the ascii characters in dataString to the SD card automatically. The code assumes you already have an text file that is 8.3 named: datalog.txt on the SD card waiting for that data save. Make this empty file with any generic text editor.

You will find an introduction to the different types of sensors that you can use with the UNO logger at:

**Arduino Tutorial: Adding Sensors to Your Data Logger**
https://thecavepearlproject.org/2017/12/17/adding-sensors-to-an-arduino-data-logger/

If you are using the UNO, check that the sensor you want to use can handle the UNO's 5V positive rail.

**Also note:** that I have a directory of little utility scripts that come in very handy when testing your Arduino based data loggers at:
https://github.com/EKMallon/Utilities
