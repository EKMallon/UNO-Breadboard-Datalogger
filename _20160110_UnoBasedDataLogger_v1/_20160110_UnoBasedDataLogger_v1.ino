/* A very simple RTC triggered datalogger script. Note that I have replaced the
analog pin reads in Tom’s Igoes starter code at //https://www.arduino.cc/en/Tutorial/Datalogger
with DS3231 I2C register reading & the delay has been replaced with sleep & RTC interrupt alarms
*/
#include <SD.h>  // I much prefer SdFat.h by Greiman over the old SD.h library used here
#include  <SPI.h>
const int chipSelect = 10;    //CS moved to pin 10 on the arduino
#include <Wire.h>
#include "LowPower.h"     // from https://github.com/rocketscream/Low-Power
#include <RTClib.h>       // library from   https://github.com/MrAlvin/RTClib
RTC_DS3231 RTC;
// creates an RTC object in the code
//variables for reading the RTC time & handling the INT(0) interrupt it generates
#define DS3231_I2C_ADDRESS 0x68
int RTC_INTERRUPT_PIN = 2;
byte Alarmhour;
byte Alarmminute;
byte Alarmday;
char CycleTimeStamp[ ] = "0000/00/00,00:00"; //16 ascii characters (without seconds)
#define SampleIntervalMinutes 1
/*
// CHANGE SampleIntervalMinutes to the number of minutes you want between samples! Whole numbers 1-59 only
// NOTE yes the RTClib will do down to one second intervals with the right code
// HOWEVER if you cut power to an SD card while it
// is writing information you destroy the data file on the card….
// so how do you solve that problem if you start sampling too frequently?
*/
volatile boolean clockInterrupt = false;  
//this flag is set to true when the RTC interrupt handler is executed

//variables for reading the DS3231 RTC temperature register
float temp3231;
byte tMSB = 0;
byte tLSB = 0;

//indicator LED pins
int RED_PIN = 4;
int GREEN_PIN = 5;
int BLUE_PIN = 6; 

void setup() {
  
  // Setting the SPI pins high helps some sd cards go into sleep mode 
  // the following pullup resistors only need to be enabled for the stand alone logger builds - not the UNO loggers
  pinMode(chipSelect, OUTPUT); digitalWrite(chipSelect, HIGH); //Always pullup the CS pin with the SD library
  //and you may need to pullup MOSI/MISO
  //pinMode(MOSIpin, OUTPUT); digitalWrite(MOSIpin, HIGH); //pullup the MOSI pin
  //pinMode(MISOpin, INPUT); digitalWrite(MISOpin, HIGH);  //pullup the MISO pin
  delay(1);
  
  Serial.begin(9600);    // Open serial communications and wait for port to open:
  Wire.begin();          // start the i2c interface for the RTC
  RTC.begin();           // start the RTC
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }
  Serial.print(F("Initializing SD card…"));
// print lines in the setup loop only happen once
// see if the card is present and can be initialized
  if (!SD.begin(chipSelect)) {
    Serial.println(F("Card failed, or not present"));
    // don’t do anything more:
    return;
  }
  Serial.println(F("card initialized."));
  
  // You must already have a plain text file file named ‘datalog.txt’ on the SD already for this to work!
  
  //————-print a header to the data file———- OPTIONAL!
  //File dataFile = SD.open("datalog.txt", FILE_WRITE);
  //if (dataFile) { // if the file is available, write to it:
  //  dataFile.println("Timestamp, DS3231 Temp(F), ");
  //I often print many extra lines of text in file headers, identifying details about the hardware being used, the code version that was running, etc
  //  dataFile.close();
  //}
  //else {
  //   Serial.println("error opening datalog.txt"); // if the file isn’t open, pop up an error:
  //}

  pinMode(RED_PIN, OUTPUT); //configure 3 RGB pins as outputs
  pinMode(GREEN_PIN, OUTPUT);
  pinMode(BLUE_PIN, OUTPUT);
  digitalWrite(RED_PIN, LOW);
  digitalWrite(GREEN_PIN, HIGH); // startup with green led lit
  digitalWrite(BLUE_PIN, LOW);
} // end of setup

void loop() {
  //—–This part reads the time and disables the RTC alarm
  DateTime now = RTC.now(); //this reads the time from the RTC
  sprintf(CycleTimeStamp, "%04d/%02d/%02d %02d:%02d", now.year(), now.month(), now.day(), now.hour(), now.minute());
  //loads the time into a string variable
  //don’t record seconds in the time stamp because
  //the interrupt to time reading interval is <1s, so seconds are always ’00’
  
  // We set the clockInterrupt in the ISR, deal with that now:
  if (clockInterrupt) {
    if (RTC.checkIfAlarm(1)) {       //Is the RTC alarm still on?
      RTC.turnOffAlarm(1);              //then turn it off.
    }
    //print (optional) debugging message to the serial window if you wish
    //Serial.print("RTC Alarm on INT-0 triggered at ");
    //Serial.println(CycleTimeStamp);
    clockInterrupt = false;                //reset the interrupt flag to false
  }//—————————————————————–
  // read the RTC temp register and print that out
  // Note: the DS3231 temp registers (11h-12h) are only updated every 64seconds
  Wire.beginTransmission(DS3231_I2C_ADDRESS);
  Wire.write(0x11);                     //the register where the temp data is stored
  Wire.endTransmission();
  Wire.requestFrom(DS3231_I2C_ADDRESS, 2);   //ask for two bytes of data
  if (Wire.available()) {
  tMSB = Wire.read();            //2’s complement int portion
  tLSB = Wire.read();             //fraction portion
  temp3231 = ((((short)tMSB << 8) | (short)tLSB) >> 6) / 4.0;  // Allows for readings below freezing: thanks to Coding Badly
  temp3231 = (temp3231 * 1.8) + 32.0; // To Convert Celcius to Fahrenheit
}
else {
  temp3231 = 0;
  //if temp3231 contains zero, then you know you had a problem reading the data from the RTC!
}
Serial.print(F(". TEMPERATURE from RTC is: "));
Serial.print(temp3231);
Serial.println(F(" Fahrenheit"));

// You could read in other variables here …like the analog pins, etc,
// and just add them to the dataString before you write it to the file

//——– concatenate data into a string ———-
// Add each piece of information to the string that gets written to the SD card with:dataFile.println(dataString);
String dataString = ""; //this line simply erases the string
dataString += CycleTimeStamp;
dataString += ", ";     //puts a comma between the two bits of data
dataString = dataString + String(temp3231);  //this only gives you two decimal places
//if you want more numbers after the decimal place replace the above line with
//char buff[10];
//dtostrf(rtc_TEMP_degC, 4, 4, buff);  //4 is mininum width, second number is #digits after the decimal
//dataString = dataString + buff;

//——– Now write the data to the SD card ——–
// open the file. note that only one file can be open at a time,
// so you have to close this one before opening another.
  File dataFile = SD.open("datalog.txt", FILE_WRITE);

  // if the file is available, write to it:
  if (dataFile) {
    dataFile.println(dataString);
    dataFile.close();
    // print to the serial port too:
    Serial.println(dataString);
  }
  // if the file isn't open, pop up an error:
  else {
    Serial.println(F("error opening datalog.txt"));
  }
  
// delay(10000);
// instead of using the delay we will use RTC interrupted sleeps

//——– Set the next alarm time ————–
Alarmhour = now.hour();
Alarmminute = now.minute() + SampleIntervalMinutes;
Alarmday = now.day();

// check for roll-overs
if (Alarmminute > 59) { //error catching the 60 rollover!
  Alarmminute = 0;
  Alarmhour = Alarmhour + 1;
  if (Alarmhour > 23) {
    Alarmhour = 0;
    // put ONCE-PER-DAY code here -it will execute on the 24 hour rollover
  }
}
// then set the alarm
RTC.setAlarm1Simple(Alarmhour, Alarmminute);
RTC.turnOnAlarm(1);
if (RTC.checkAlarmEnabled(1)) {
  //you would comment out most of this message printing
  //if your logger was actually being deployed in the field
  Serial.print(F("RTC Alarm Enabled!"));
  Serial.print(F(" Going to sleep for : "));
  Serial.print(SampleIntervalMinutes);
  Serial.println(F(" minutes"));
  Serial.println();Serial.flush();        //just adds a carriage return
}
delay(100); //this delay is only here so we can see the LED’s it is totally optional!
  //otherwise the entire loop would execute too fast for us to see it!
  digitalWrite(GREEN_PIN, LOW);
  digitalWrite(RED_PIN, HIGH);
  // Turn on red led as our indicator that the Arduino is sleeping.
  // Note: Normally you would NOT leave an LED on like this during sleep! This is just so you can see what is going on..
  
  //——– sleep and wait for next RTC alarm ————–
  // Enable interrupt on pin2 & attach it to rtcISR function:
  attachInterrupt(0, rtcISR, LOW);
  // Enter power down state with ADC module disabled to save power:
  LowPower.powerDown(SLEEP_FOREVER, ADC_OFF, BOD_ON);
  //processor starts HERE AFTER THE RTC ALARM WAKES IT UP
  detachInterrupt(0); // immediately disable the interrupt on waking
  digitalWrite(RED_PIN, LOW);
  digitalWrite(GREEN_PIN, HIGH); //Interupt woke processor, turn on green led

}// this is the END of the MAIN LOOP
  // This is the Interrupt subroutine that only executes when the rtc alarm goes off

void rtcISR() {
    clockInterrupt = true;
  }
