
/* A basic RTC triggered datalogger script. Note that I have replaced the
analog pin reads in Tom’s Igoes starter code from //https://www.arduino.cc/en/Tutorial/Datalogger
with DS3231 I2C register reading & the delay has been replaced with sleep & RTC interrupt alarms
*/
#include <SD.h>  // I much prefer SdFat.h by Greiman over the old SD.h library used here
#include  <SPI.h>
const int chipSelect = 10;    //CS moved to pin 10 on the arduino
#include <Wire.h>
#include "LowPower.h"     // from https://github.com/rocketscream/Low-Power
#include <RTClib.h>       // library from https://github.com/MrAlvin/RTClib  there are many other DS3231 libs availiable
RTC_DS3231 RTC;
// creates an RTC object in the code
// variables for reading the RTC time & handling the INT(0) interrupt it generates
#define DS3231_I2C_ADDRESS 0x68
int RTC_INTERRUPT_PIN = 2;
byte Alarmhour;
byte Alarmminute;
byte Alarmday;
char CycleTimeStamp[ ] = "0000/00/00,00:00"; //16 ascii characters (without seconds)
#define SampleIntervalMinutes 1  // Whole numbers 1-30 only, must be a divisor of 60
// CHANGE SampleIntervalMinutes to the number of minutes you want between samples!
// NOTE: the RTC will go down to one second alarm intervals with additonal code

volatile boolean clockInterrupt = false;  
//this flag is set to true when the RTC interrupt handler is executed

const char codebuild[] PROGMEM = __FILE__;  // loads the compiled source code directory & filename into a varaible
const char header[] PROGMEM = "Timestamp,RTC temp(C),Analog(A0),Add more headers here"; //gets written to second line datalog.txt in setup

//variables for reading the DS3231 RTC temperature register
float temp3231;
byte tMSB = 0;
byte tLSB = 0;

//variables for analog pin reading
int analogPin = 3;
int valA0 = 0;

//indicator LED pins - change to suit your connections
int RED_PIN = 4;
int GREEN_PIN = 5;
int BLUE_PIN = 6; 

void setup() {
  
  pinMode(RTC_INTERRUPT_PIN,INPUT_PULLUP);//RTC alarms low, so need pullup on the D2 line 
  //Note the above line is not needed if you have hardware pullups on SQW line, and many RTC modules do.
  
  // Setting the SPI pins high helps some sd cards go into sleep mode 
  // the following pullup resistors only needs to be enabled for the ProMini builds - not the UNO loggers
  pinMode(chipSelect, OUTPUT); digitalWrite(chipSelect, HIGH); //ALWAYS pullup the ChipSelect pin with the SD library
  //and you may need to pullup MOSI/MISO, usually MOSIpin=11, and MISOpin=12
  //pinMode(MOSIpin, OUTPUT); digitalWrite(MOSIpin, HIGH); //pullup the MOSI pin
  //pinMode(MISOpin, INPUT); digitalWrite(MISOpin, HIGH);  //pullup the MISO pin
  delay(1);
  
  Serial.begin(9600);    // Open serial communications and wait for port to open:
  Wire.begin();          // start the i2c interface for the RTC
  RTC.begin();           // start the RTC

  // check RTC Status
  //****************
  clearClockTrigger(); //stops RTC from holding the interrupt low if system reset just occured
  RTC.turnOffAlarm(1);
  DateTime now = RTC.now();
  sprintf(CycleTimeStamp, "%04d/%02d/%02d %02d:%02d", now.year(), now.month(), now.day(), now.hour(), now.minute());
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
  File dataFile = SD.open("datalog.txt", FILE_WRITE);
  if (dataFile) { // if the file is available, write to it:
    dataFile.println((__FlashStringHelper*)codebuild); // writes the entire path + filename to the start of the data file
    dataFile.println((__FlashStringHelper*)header);
    dataFile.close();
  }
  else {
     Serial.println("error opening datalog.txt"); // if the file isn’t open, pop up an error:
  }

  pinMode(RED_PIN, OUTPUT); //configure 3 RGB pins as outputs
  pinMode(GREEN_PIN, OUTPUT);
  pinMode(BLUE_PIN, OUTPUT);
  digitalWrite(RED_PIN, LOW);
  digitalWrite(GREEN_PIN, HIGH); // startup with green led lit
  digitalWrite(BLUE_PIN, LOW);
}
//============end of setup=================

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

// You could read in other variables here …like the analog pins, I2C sensors, etc
// and just add them to the dataString before you write it to the file
valA0 = analogRead(analogPin); 
// for stand-alone ProMini loggers, I monitor the main battery voltage (which is > Aref)
// with a voltage divider: RawBattery - 10MΩ - A0 - 3.3MΩ - GND
// with a 104 ceramic capacitor accross the 3.3MΩ resistor to enable the ADC to read the high impedance resistors
// Then calculate: float batteryVoltage = float((analogRead(A0)/ 255.75)*3.3);

//=========concatenate data into a string =====================
// Add each piece of information to the string that gets written to the SD card with:dataFile.println(dataString);
String dataString = ""; //this line simply erases the string
dataString = dataString + CycleTimeStamp;
dataString = dataString + ", ";     //separate data with a comma so they can be imported to separate collumns in a spreadsheet
dataString = dataString + String(temp3231);  //this only gives you two decimal places
//With floating point nubmers, if you want more digits after the decimal place replace the above line with these three:
//char buff[10];
//dtostrf(rtc_TEMP_degC, 4, 4, buff);  //4 is mininum width, second number is #digits after the decimal
//dataString = dataString + buff;
dataString = dataString + ", "; 
dataString = dataString + String(valA0);

//========== Now write the data to the SD card ===========
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
  
//============Set the next alarm time =============
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
  Serial.println();Serial.flush();//adds a carriage return & waits for buffer to empty
}

  delay(25); //this optional delay is only here so we can see the LED’s otherwise the entire loop executes so fast you might not see it.
  digitalWrite(GREEN_PIN, LOW);
  // Note: Normally you would NOT leave a red indicator LED on during sleep! This is just so you can see when your logger is sleeping, & when it's awake
  //digitalWrite(RED_PIN, HIGH);  // Turn on red led as our indicator that the Arduino is sleeping. Remove this before deployment.
  //——– sleep and wait for next RTC alarm ————–
  // Enable interrupt on pin2 & attach it to rtcISR function:
  attachInterrupt(0, rtcISR, LOW);
  // Enter power down state with ADC module disabled to save power:
  LowPower.powerDown(SLEEP_FOREVER, ADC_OFF, BOD_ON);
  //processor starts HERE AFTER THE RTC ALARM WAKES IT UP
  detachInterrupt(0); // immediately disable the interrupt on waking
  //digitalWrite(RED_PIN, LOW);
  digitalWrite(GREEN_PIN, HIGH); //Interupt woke processor, turn on green led
}
//================ END of the MAIN LOOP ===============

// This is the Interrupt subroutine that only executes when the RTC alarm goes off
void rtcISR() {
    clockInterrupt = true;
  }

void clearClockTrigger()  // from  http://forum.arduino.cc/index.php?topic=109062.0
{
  byte bytebuffer1=0;
  Wire.beginTransmission(0x68);   //Tell devices on the bus we are talking to the DS3231
  Wire.write(0x0F);               //Tell the device which address we want to read or write
  Wire.endTransmission();         //Before you can write to and clear the alarm flag you have to read the flag first!
  Wire.requestFrom(0x68,1);       //Read one byte
  bytebuffer1=Wire.read();        //In this example we are not interest in actually using the bye
  Wire.beginTransmission(0x68);   //Tell devices on the bus we are talking to the DS3231 
  Wire.write(0x0F);               //Status Register: Bit 3: zero disables 32kHz, Bit 7: zero enables the main oscilator
  Wire.write(0b00000000);         //Write the byte.  //Bit1: zero clears Alarm 2 Flag (A2F), Bit 0: zero clears Alarm 1 Flag (A1F)
  Wire.endTransmission();
  clockInterrupt=false;           //Finally clear the flag we use to indicate the trigger occurred
}
