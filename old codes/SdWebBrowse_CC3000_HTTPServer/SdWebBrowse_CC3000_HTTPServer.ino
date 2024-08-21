/******************************************** 
  ■ SdWebBrowse_CC3000_HTTPServer.ino       ■
  ■ Updated 01/26/2017 05:18 PM EST         ■
  ■ Using Arduino Mega 2560,                ■
  ■ Adafruit CC3000 Shield, DS1307,         ■
  ■ DHT22, and BMP085.                      ■
  ■                                         ■
  ■ Based on Adafruit CC3000 library        ■
  ■ Example:  HTTPServer.ino                ■
  ■                                         ■
  ■ Modified Sketch by "tech500" with       ■
  ■ help from "Adafruit Forum,"             ■
  ■ "Arduino.cc forum," and                 ■
  ■ "Arduino Stack Exchange."               ■

Revised routine used for resetting the "Status Bit Memory"  lines 313 to 362

Removed client IP feature 12/20/2016 --observed difficulty connecting to server; taking multiple attempts to connect.  Returned to Adafruit_CC3000 Library.
No modified files required.

Improved flow control --added fileDownload = 1 to Weather section of listen function
to allow competion of Weather Observation HTML and allow connection to close

Replaced minuteCall() with watchDog()

Added listen() to end of init_network(); instances of init_network called from Loop hanging sketch

Added two watchDog() calls to init_network function to keep WDT active

Added watchDog() call to fileRead function after sending 1000 client.writes to keep WDT alive


****************************************************/


// ********************************************************************************
// ********************************************************************************
//
//   See invidual library downloads for each library license.
//
//   Following code was developed using the Adafruit CC300 library, "HTTPServer" example.
//   and by merging library examples, adding logic for Sketch flow.
//
// *********************************************************************************
// *********************************************************************************

#include <Adafruit_CC3000.h>   //https://github.com/adafruit/Adafruit_CC3000_Library
#include <Adafruit_CC3000_Server.h>   //https://github.com/adafruit/Adafruit_CC3000_Library (Modified file to aquire clientIP)
#include "utility/debug.h"   //https://github.com/adafruit/Adafruit_CC3000_Library
#include <SdFat.h>   //https://github.com/greiman/SdFat
#include <SdFile.h>   //https://github.com/greiman/SdFat
#include <SdFatUtil.h>   //https://github.com/greiman/SdFat
#include <Wire.h>    //  http://arduino.cc/en/Main/Software  included in Arduino IDE download
#include <BMP085.h>      // http://code.google.com/p/bmp085driver/
#include <DHT.h>   //https://github.com/adafruit/DHT-sensor-library
#include <RTCTimedEvent.h>   // http://code.google.com/p/ebl-arduino/wiki/RTCTimedEvent
#include <LiquidCrystal_I2C.h>   //https://bitbucket.org/fmalpartida/new-liquidcrystal/downloads/LiquidCrystal_V1.2.1.zip
#include <SPI.h>   //  http://arduino.cc/en/Main/Software  included in Arduino IDE download

//Real Time Clock used  DS1307

//use I2Cscanner to find LCD display address, in this case 3F   //https://github.com/todbot/arduino-i2c-scanner/
LiquidCrystal_I2C lcd(0x3F,16,2);  // set the LCD address to 0x3F for a 16 chars and 2 line display

// SD chip select pin
const uint8_t chipSelect = 4;

// file system
SdFat sd;

// Serial print stream
ArduinoOutStream cout(Serial);
//------------------------------------------------------------------------------
// store error strings in flash to save RAM
#define error(s) sd.errorHalt_P(PSTR(s))
//------------------------------------------------------------------------------

String logFileName;    //String object for constructing log file name from current date


long Temperature = 0, Pressure = 0, Altitude = 0;

#define DHTPIN 7     // DHT22, pin 2 connected to this Arduino pin 7
// Uncomment whatever type you're using!
//#define DHTTYPE DHT11   // DHT 11
#define DHTTYPE DHT22   // DHT 22  (AM2302) 
//#define DHTTYPE DHT21   // DHT 21 (AM2301)

// DHT22 connections
// Connect pin 1 (on the left) of the sensor to +5V
// Connect pin 2 of the sensor to whatever your DHTPIN is
// Connect pin 4 (on the right) of the sensor to GROUND
// Connect a 10K resistor from pin 2 (data) to pin 1 (power) of the sensor

DHT dht(DHTPIN, DHTTYPE);

#define sonalertPin 9  // pin for Piezo buzzer

float h;   // humidity
float t;   // temperature C.
float f;   // temperature F.
double dewPoint;   // dew point
float hi;  //heat index in degrees F.

#define BUFSIZE 64  //Size of read buffer for file download  -optimized for CC3000.

BMP085 dps = BMP085();      // Digital Pressure Sensor BMP085, Model GY-65 purchased on EBay  (Vcc = 5 volts)

float pressure;
float currentPressure;  //Present pressure reading used to find pressure change difference.
float pastPressure;  //Previous pressure reading used to find pressure change difference.
float milliBars;

float difference;

#define RESET_WATCHDOG1 33 //SwitchDoc Labs external Watchdog Dual Timer, JP7
#define Q 43 //74HCT73 Q 
#define RESET 38 //74HCT73 Clear

//JP1 goes LOW to reset Arduino Mega

int value;  //Status of 74HCT73, Q pin  Not set = 0 and if Q is set = 1.

//long int id = 1;  //Increments record number
char *filename;
char str[16] = {0};

int timer;

String dtStamp;
String lastUpdate;
String SMonth, SDay, SYear, SHour, SMin, SSec;
String reConnect;
String fileRead;


// These are the interrupt and control pins
#define ADAFRUIT_CC3000_IRQ   3  // MUST be an interrupt pin!
// These can be any two pins
#define ADAFRUIT_CC3000_VBAT  5
#define ADAFRUIT_CC3000_CS    10
// Use hardware SPI for the remaining pins
// On an UNO, SCK = 13, MISO = 12, and MOSI = 11

Adafruit_CC3000 cc3000 = Adafruit_CC3000(ADAFRUIT_CC3000_CS, ADAFRUIT_CC3000_IRQ, ADAFRUIT_CC3000_VBAT,
                         SPI_CLOCK_DIVIDER); // you can change this clock speed

// Local server IP, port
uint32_t ip = cc3000.IP2U32(10,0,0,49);


#define WLAN_SSID       "Security-22"   // cannot be longer than 32 characters!
#define WLAN_PASS       "1048acdc7388"
// Security can be WLAN_SEC_UNSEC, WLAN_SEC_WEP, WLAN_SEC_WPA or WLAN_SEC_WPA2
#define WLAN_SECURITY   WLAN_SEC_WPA2

#define LISTEN_PORT           8001    // What TCP port to listen on for connections.  
// The HTTP protocol uses port 80 by default.

#define MAX_ACTION            10      // Maximum length of the HTTP action that can be parsed.

#define MAX_PATH              64      // Maximum length of the HTTP request path that can be parsed.
// There isn't much memory available so keep this short!

#define BUFFER_SIZE           MAX_ACTION + MAX_PATH + 20  // Size of buffer for incoming request data.
// Since only the first line is parsed this
// needs to be as large as the maximum action
// and path plus a little for whitespace and
// HTTP version.

#define TIMEOUT_MS           250   // Amount of time in milliseconds to wait for     /////////default 500/////
// an incoming request to finish.  Don't set this
// too high or your server could be slow to respond.

Adafruit_CC3000_Server httpServer(LISTEN_PORT);

uint8_t buffer[BUFFER_SIZE+1];
int bufindex = 0;
char action[MAX_ACTION+1];
char path[MAX_PATH+1];

Sd2Card card;
SdVolume volume;
SdFile root;
SdFile file;

int fileDownload;   //File download status; 1 = file download has started, 0 = file has finished downloading

char MyBuffer[13];

// store error strings in flash to save RAM
#define error(s) error_P(PSTR(s))

void error_P(const char* str)
{
     Serial.println("error: ");
     Serial.println(str);

     if (card.errorCode())
     {
          Serial.println("SD error: ");
          Serial.print(card.errorCode(), HEX);
          Serial.print(',');
          Serial.println(card.errorData(), HEX);
     }

     while(1);
}

////////////////
void setup(void)
{

     getDateTime();
     
     watchDog();   //added 12/28/2016
     
     delay(1000 * 10);   //wait for Serial Monitor

     Serial.begin(115200);
     
     Serial.println("\nRESET has occured:  " + dtStamp);

     pinMode(sonalertPin, OUTPUT);  //Used for Piezo buzzer

     pinMode(Q, INPUT_PULLUP);  //Monitoring status of 74HC73, Q Output
     
     pinMode(RESET, OUTPUT);
     
     Wire.begin();

     sd.begin(chipSelect);

     dht.begin();

     lcd.init();
     
     value = digitalRead(Q);  //Status of 74HCT73, Q Output, value = 1 --if Q is HIGH
     
     //Serial.print("Free RAM: ");
     //Serial.println(FreeRam());

     // initialize the SD card at SPI_HALF_SPEED to avoid bus errors with
     // breadboards.  use SPI_FULL_SPEED for better performance.
     pinMode(10, OUTPUT);                       // set the SS pin as an output (necessary!)
     digitalWrite(10, HIGH);                    // but turn off the W5100 chip!

     if (!card.init(SPI_FULL_SPEED, 4)) error("card.init failed!");

     // initialize a FAT volume
     if (!volume.init(&card)) error("vol.init failed!");

     Serial.print("\nVolume is FAT");
     Serial.println(volume.fatType(),DEC);
     Serial.println();

     if (!root.openRoot(&volume)) error("openRoot failed");

     /*
       // list file in root with date and size
       Serial.println("Files found in root:");
       root.ls(LS_DATE | LS_SIZE);
       Serial.println();

       // Recursive list of all directories
       PgmPrintln("Files found in all dirs:");
       root.ls(LS_R);

       Serial.println();
       Serial.println("Done");
     */

     Serial.println(F("Hello, CC3000!\n"));

     // Initialise the module
     Serial.println(F("\nInitializing CC3000..."));
     if (!cc3000.begin())
     {
          Serial.println(F("Couldn't begin()! Check your wiring?"));
          while(1);
     }

     Serial.print(F("\nAttempting to connect to "));
     Serial.println(WLAN_SSID);

     if (!cc3000.connectToAP(WLAN_SSID, WLAN_PASS, WLAN_SECURITY))
     {
          Serial.println(F("Failed!"));
          while(1);
     }

     Serial.println(F("Connected!"));

     Serial.println(F("Request DHCP"));

     while (!cc3000.checkDHCP())
     {
          delay(100); // ToDo: Insert a DHCP timeout!
     }

     // Display the IP address DNS, Gateway, etc.
     while (! displayConnectionDetails())
     {
          delay(1000);
     }

     Serial.println("");

     // Start listening for connections
     httpServer.begin();



/////////////// J-K Flip-Flop 74HCT73, Q Output of Status Bit Memory ///////////////////////////////////////

     delay(100);

     getDateTime();

     if(value == 1)
     {

          //Creates an entry in "Server.txt" for every RESET cause by "Dual Watchdog Timer"
          SdFile serverFile;
          serverFile.open("Server.txt", O_RDWR | O_CREAT | O_APPEND);

          if (!serverFile.isOpen()) error("Watchdog Start Server");

          serverFile.println("Watchdog RESET:  " + dtStamp);
          serverFile.close();
          Serial.print("Watchdog RESET  ");
          Serial.println(dtStamp + "  ");
          Serial.println("Listening for connections...  ");
          //Serial.println(value);
          
          //Sends LOW to RESET the 74HCT73, JK Flip-flop
          digitalWrite(RESET, LOW);
          delay(100);
          digitalWrite(RESET, HIGH);
          delay(100);
          digitalWrite(RESET, LOW);

     }
     else
     {

          //Creates an entry in "Server.txt" for every RESET; caused by opening Serial Monitor
          SdFile serverFile;
          serverFile.open("Server.txt", O_RDWR | O_CREAT | O_APPEND);

          if (!serverFile.isOpen()) error("Manual Start Server");

          serverFile.println("Manual RESET:  " + dtStamp);
          serverFile.close();
          Serial.print("Manual RESET  ");
          Serial.println(dtStamp + "  ");
          Serial.println("Listening for connections...  ");
          //Serial.println(value);
          
     }
     

///////////////////////////////////////////////////////////////////////////////////////////////



     //Uncomment to set Real Time Clock --only needs to be run once

     /*
     //Set Time and Date of the DS1307 Real Time Clock
     RTCTimedEvent.time.second = 00;
     RTCTimedEvent.time.minute = 48;
     RTCTimedEvent.time.hour = 15;
     RTCTimedEvent.time.dayOfWeek  = 2;
     RTCTimedEvent.time.day = 6;
     RTCTimedEvent.time.month = 6;
     RTCTimedEvent.time.year = 2016;
     RTCTimedEvent.writeRTC();
     */

     /*
     //initial buffer timer
     RTCTimedEvent.initialCapacity = sizeof(RTCTimerInformation)*3;

     //event for every minute
     RTCTimedEvent.addTimer(TIMER_ANY, //minute
     TIMER_ANY, //hour
     TIMER_ANY, //day fo week
     TIMER_ANY, //day
     TIMER_ANY, //month
     minuteCall);
     */
  
     // uncomment for different initialization settings
     //dps.init();     // QFE (Field Elevation above ground level) is set to 0 meters.
     // same as init(MODE_STANDARD, 0, true);

     //dps.init(MODE_STANDARD, 101850, false);  // 101850Pa = 1018.50hPa, false = using Pa units
     // this initialization is useful for normalizing pressure to specific datum.
     // OR setting current local hPa information from a weather station/local airport (QNH).

     dps.init(MODE_ULTRA_HIGHRES, 25115.5, true);  // 824 Ft. GPS indicated Elevation, true = using meter units
     // this initialization is useful if current altitude is known,
     // pressure will be calculated based on TruePressure and known altitude.

     // note: use zeroCal only after initialization.
     // dps.zeroCal(101800, 0);    // set zero point

     getDateTime();

     getDHT22();

     getBMP085();

     //lcdDisplay();      //   LCD 1602 Display function --used for inital display

     Serial.flush();
     Serial.end();

}

///////////////////////////////////////////////////////////////////////////
char ListFiles(Adafruit_CC3000_ClientRef client, uint8_t flags, SdFile dir)
{
     // This code is just copied from SdFile.cpp in the SDFat library
     // and tweaked to print to the client output in html!
     dir_t p;

     dir.rewind();
     client.println("<ul>");

     while (dir.readDir(&p) > 0)
     {
          // done if past last used entry
          if (p.name[0] == DIR_NAME_FREE) break;

          // skip deleted entry and entries for . and  ..
          if (p.name[0] == DIR_NAME_DELETED || p.name[0] == '.') continue;

          // only list subdirectories and files
          if (!DIR_IS_FILE_OR_SUBDIR(&p)) continue;

          // print any indent spaces
          client.print("<li><a href=\"");
          for (uint8_t i = 0; i < 11; i++)
          {
               if (p.name[i] == ' ') continue;
               if (i == 8)
               {
                    client.print('.');
               }
               client.print((char)p.name[i]);
          }
          if (DIR_IS_SUBDIR(&p))
          {
               client.print('/');
          }
          client.print("\">");

          // print file name with possible blank fill
          for (uint8_t i = 0; i < 11; i++)
          {
               if (p.name[i] == ' ') continue;
               if (i == 8)
               {
                    client.print('.');
               }
               client.print((char)p.name[i]);
          }

          if (DIR_IS_SUBDIR(&p))
          {
               client.print('/');
          }
          client.print("</a>");



          // print modify date/time if requested
          if (flags & LS_DATE)
          {
               dir.printFatDate(p.lastWriteDate);
               client.print(' ');
               dir.printFatTime(p.lastWriteTime);
          }
          // print size if requested
          if (!DIR_IS_SUBDIR(&p) && (flags & LS_SIZE))
          {
               client.print(' ');
               client.print(p.fileSize);
          }
          client.println("</li>");
     }
     client.println("</ul>");
}

// How big our line buffer should be. 100 is plenty!
#define BUFFER 100


///////////
void loop()
{

     watchDog();  //Keep SwitchDoc Labs WDT alive; prevents resetting of the Arduino, otherwise WDT generates an Arduino reset
     
     //  check wireless lan connective --if needed re-establish connection
     if (!cc3000.checkConnected())      // make sure still connected to wireless network
     {

          reConnect = "";
          reConnect = "Loop";

          if (!init_network())    // reconnect to WLAN
          {
               delay(15 * 1000); // if no connection, try again later
               return;
          }
     }
     
          fileDownload = 0;

          RTCTimedEvent.loop();
          delay(50);
          RTCTimedEvent.readRTC();
          delay(50);

          //Collect  "log.txt" Data for one day; do it early so day of week still equals 7
          if (((RTCTimedEvent.time.hour) == 23 )  &&
               ((RTCTimedEvent.time.minute) == 58) &&
               ((RTCTimedEvent.time.second) == 00))
          {
               newDay();
          }

          //Write Data at 15 minute interval

          if ((((RTCTimedEvent.time.minute) == 0)||
               ((RTCTimedEvent.time.minute) == 15)||
               ((RTCTimedEvent.time.minute) == 30)||
               ((RTCTimedEvent.time.minute) == 45))
               && ((RTCTimedEvent.time.second) == 00))
          {

               

               getDateTime();

               lastUpdate = dtStamp;   //store dtstamp for use on dynamic web page

               getDHT22();

               getBMP085();

               updateDifference();  //Get Barometric Pressure difference

               logtoSD();   //Output to SD Card  --Log to SD on 15 minute interval.

               delay(100);  //Be sure there is enough SD write time

               //lcdDisplay();      //   LCD 1602 Display function --used for 15 minute update

               pastPressure = (Pressure *  0.000295333727);   //convert to inches mercury

          }
          else
          {
               listen();  //Listen for web client
          }
     
}

//////////////
void logtoSD()   //Output to SD Card every fifthteen minutes
{

     Serial.begin(115200);

     if(fileDownload == 1)   //File download has started
     {
          exit;   //Skip logging this time --file download in progress
     }
     else
     {

          fileDownload = 1;
          
          // Open a "log.txt" for appended writing
          SdFile logFile;
          logFile.open("log.txt", O_WRITE | O_CREAT | O_APPEND);
          if (!logFile.isOpen()) error("log");

          //logFile.print(id);
          //logFile.print(" , ");
          logFile.print(dtStamp) + " EST";
          logFile.print(" , ");
          logFile.print("Humidity:  ");
          logFile.print(h);
          logFile.print(" % , ");
          logFile.print("Dew Point:  ");
          logFile.print((dewPoint) + (9/5 + 32)); 
          logFile.print(" F. , ");
          logFile.print(f);
          logFile.print("  F. , ");
          // Reading temperature or humidity takes about 250 milliseconds!
          // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
          logFile.print("Heat Index:  ");
          logFile.print(hi);
          logFile.print(" F. ");
          logFile.print(" , ");
          //logFile.print((Pressure *  0.000295333727), 3);  //Convert Pascals to inches of Mecury
          logFile.print(currentPressure,3);
          logFile.print(" in. Hg. ");
          logFile.print(" , ");

          if (pastPressure == currentPressure)
          {
               logFile.print("0.000");
               logFile.print(" Difference ");
               logFile.print(" ,");
          }
          else
          {
               logFile.print((difference),3);
               logFile.print(" Difference ");
               logFile.print(", ");
          }

          logFile.print(milliBars,3);  //Convert Pascals to millibars
          logFile.print(" millibars ");
          logFile.print(" , ");
          logFile.print((Pressure * 0.00000986923267), 3);   //Convert Pascals to Atm (atmospheric pressure)
          logFile.print(" atm ");
          logFile.print(" , ");
          logFile.print(Altitude * 0.0328084);  //Convert cm to Feet
          logFile.print(" Ft. ");
          logFile.println();
          //Increment Record ID number
          //id++;
          Serial.println("\nData written to logFile  " + dtStamp);
          
          logFile.close();
          
          fileDownload = 0;

          if(abs(difference) >= .020)  //After testing and observations of Data; raised from .010 to .020 inches of Mecury
          {
               // Open a "Differ.txt" for appended writing --records Barometric Pressure change difference and time stamps
               SdFile diffFile;
               diffFile.open("Differ.txt", O_WRITE | O_CREAT | O_APPEND);

               if (!diffFile.isOpen()) error("diff"); 
               {

                    Serial.println("");
                    Serial.print("Difference greater than .020 inches of Mecury ,  ");
                    Serial.print(difference, 3);
                    Serial.print("  ,");

                    diffFile.println("");
                    diffFile.print("Difference greater than .020 inches of Mecury,  ");
                    diffFile.print(difference, 3);
                    diffFile.print("  ,");
                    diffFile.print(dtStamp);
                    diffFile.close();

                    beep(50);  //Duration of Sonalert tone

               }
               
               //  check wireless lan connective --if needed re-establish connection
               if (!cc3000.checkConnected())      // make sure still connected to wireless network
               {

                    reConnect = "";
                    reConnect = "logtoSD";

                    if (!init_network())    // reconnect to WLAN
                    {
                         delay(15 * 1000); // if no connection, try again later
                         return;
                    }
               }
               
          }
          Serial.flush();
          Serial.end();    
     }
     listen();
}

/////////////////
void lcdDisplay()   //   LCD 1602 Display function
{

     lcd.clear();
     // set up the LCD's number of rows and columns:
     lcd.backlight();
     lcd.noAutoscroll();
     lcd.setCursor(0, 0);
     // Print Barometric Pressure
     lcd.print((Pressure *  0.000295333727),3);   //convert to inches mercury
     lcd.print(" in. Hg.");
     // set the cursor to column 0, line 1
     // (note: line 1 is the second row, since counting begins with 0):
     lcd.setCursor(0, 1);
     // print millibars
     lcd.print(((Pressure) * .01),3);   //convert to millibars
     lcd.print(" mb.    ");
     lcd.print("");

}

/////////////
void listen()   // Listen for client connection
{



     fileDownload = 0;   //No file being downloaded

     Adafruit_CC3000_ClientRef client = httpServer.available();

     while(client.connected())
     {

          if (client)
          {

               // Process this request until it completes or times out.
               // Note that this is explicitly limited to handling one request at a time!

               // Clear the incoming data buffer and point to the beginning of it.
               bufindex = 0;
               memset(&buffer, 0, sizeof(buffer));

               // Clear action and path strings.
               memset(&action, 0, sizeof(action));
               memset(&path,   0, sizeof(path));

               // Set a timeout for reading all the incoming data.
               unsigned long endtime = millis() + TIMEOUT_MS;

               // Read all the incoming data until it can be parsed or the timeout expires.
               bool parsed = false;

               while (!parsed && (millis() < endtime) && (bufindex < BUFFER_SIZE))
               {

                    if (client.available())
                    {
                         
                         buffer[bufindex++] = client.read();
                         
                    }

                    parsed = parseRequest(buffer, bufindex, action, path);
                    
               }
                    
                    if(parsed)
                    {
                         
                         Serial.begin(115200);
                         Serial.println();
                         getDateTime();
                         Serial.println("Client connected:  " + dtStamp);
                         Serial.println(F("Processing request"));
                         Serial.print(F("Action: "));
                         Serial.println(action); 
                         Serial.print(F("Path: "));
                         Serial.println(path);

                         /*
                         /////////////////////////////////////////////////////////////////////////////////////////////////////////////////
                         //  Requires modified Adafruit_CC3000 Library; not available using Adafruit_CC3000 Library without modifications.
                         //  to get client IP address
                         /////////////////////////////////////////////////////////////////////////////////////////////////////////////////
                         char ip1String[] = "10.0.0.146";   //Host ip address
                         char ip2String[16] = "";   //client.ip_addr = clientIP[16]
                         snprintf(ip2String, 16, "%d.%d.%d.%d", client.ip_addr[3], client.ip_addr[2], client.ip_addr[1], client.ip_addr[0]);

                         //Serial.print("Client IP:  ");
                         //Serial.println(ip2String);
                         */

                         // Open a "access.txt" for appended writing.   Client access ip address logged.
                         SdFile logFile;
                         logFile.open("access.txt", O_WRITE | O_CREAT | O_APPEND);

                         /*
                         if (!logFile.isOpen()) error("log");

                         if (0 == (strncmp(ip1String, ip2String, 16)))
                         {
                              //Serial.println("addresses match");
                              exit;
                         }
                         else
                         {
                         */
                              //Serial.println("addresses that do not match ->log client ip address");

                              logFile.print("Accessed:  ");
                              logFile.print(dtStamp + " , ");
                              //logFile.print("Client IP:  ");
                              //logFile.print(ip2String);
                              //logFile.print(" -- ");
                              logFile.print("Path:  ");
                              logFile.print(path);
                              logFile.println("");
                              logFile.close();
                         //}
                         exit;
                        
                         //////////////////////////////////////////////////////////////////////////////////////

                         // Check the action to see if it was a GET request.
                         if (strncmp(path, "/Weather", 8) == 0)   // Respond with the path that was accessed.
                         {

                              fileDownload = 1;
                              
                              // First send the success response code.
                              client.fastrprintln(F("HTTP/1.1 200 OK"));
                              client.fastrprintln(F("Content-Type: html"));
                              client.fastrprintln(F("Connnection: close"));
                              client.fastrprintln(F("Server: Adafruit CC3000"));
                              // Send an empty line to signal start of body.
                              client.fastrprintln(F(""));
                              // Now send the response data.
                              // output dynamic webpage
                              client.fastrprintln(F("<!DOCTYPE HTML>"));
                              client.fastrprintln(F("<html>\r\n"));
                              client.fastrprintln(F("<body>\r\n"));
                              client.fastrprintln(F("<head><title>Weather Observations</title></head>"));
                              // add a meta refresh tag, so the browser pulls again every 15 seconds:
                              //client.fastrprintln(F("<meta http-equiv=\"refresh\" content=\"15\">"));
                              client.fastrprintln(F("<h2>Treyburn Lakes</h2><br />"));
                              client.fastrprintln(F("Indianapolis, IN 46239<br />"));

                              if(lastUpdate != NULL)
                              {
                                   client.println("Last Update:  ");
                                   client.println(lastUpdate);
                                   client.println(" EST <br />");
                              }

                              client.fastrprintln(F("Humidity:  "));
                              client.print(h, 2); 
                              client.fastrprint(F(" %<br />"));
                              client.fastrprintln(F("Dew point:  "));
                              client.print((dewPoint) + (9/5 +32),1);
                              client.fastrprint(F(" F. <br />"));
                              client.fastrprintln(F("Temperature:  "));
                              client.print(f);
                              client.fastrprint(F(" F.<br />"));
                              client.fastrprintln(F("Heat Index:  "));
                              client.print(hi);
                              client.fastrprint(F(" F. <br />"));
                              client.fastrprintln(F("Barometric Pressure:  "));
                              client.print(currentPressure);
                              client.fastrprint(F(" in. Hg.<br />"));

                              if (pastPressure == currentPressure)
                              {
                                   client.println(difference, 3);
                                   client.fastrprint(F(" Difference in. Hg <br />"));
                              }
                              else
                              {
                                   client.println(difference, 3);
                                   client.fastrprint(F(" Difference in. Hg <br />"));
                              }

                              client.fastrprintln(F("Barometric Pressure:  "));
                              client.println(milliBars);
                              client.fastrprintln(F(" mb.<br />"));
                              client.fastrprintln(F("Atmosphere:  "));
                              client.print(Pressure * 0.00000986923267, 3);   //Convert Pascals to Atm (atmospheric pressure)
                              client.fastrprint(F(" atm <br />"));
                              client.fastrprintln(F("Altitude:  "));
                              client.print(Altitude * 0.0328084, 2);  //Convert cm to Feet
                              client.fastrprint(F(" Feet<br />"));
                              client.fastrprintln(F("<br /><br />"));
                              client.fastrprintln(F("<h2>Collected Observations</h2>"));
                              client.println("<a href=http://69.245.183.113:8001/LOG.TXT download>Current Week Observations</a><br />");
                              client.fastrprintln(F("<br />\r\n"));
                              client.println("<a href=http://69.245.183.113:8001/SdBrowse >Weekly Data Files</a><br />");
                              client.fastrprintln(F("<br />\r\n"));
                              client.println("<a href=http://69.245.183.113:8001/README.TXT download>Server:  README</a><br />");
                              client.fastrprintln(F("<body />\r\n"));
                              client.fastrprintln(F("<br />\r\n"));
                              client.fastrprintln(F("</html>\r\n"));
                              
                              fileDownload = 0;

                         }
                         // Check the action to see if it was a GET request.
                         else if (strcmp(path, "/SdBrowse") == 0) // Respond with the path that was accessed.
                         {

                              fileDownload = 1;

                              // send a standard http response header
                              client.println("HTTP/1.1 200 OK");
                              client.println("Content-Type: text/html");
                              client.println();
                              client.println("<!DOCTYPE HTML>");
                              client.println("<html>\r\n");
                              client.println("<body>\r\n");
                              client.println("<head><title>SDBrowse</title><head />");
                              // print all the files, use a helper to keep it clean
                              client.println("<h2>Server Files:</h2>");
                              ListFiles(client, LS_SIZE, root);
                              client.println("\n<a href=http://69.245.183.113:8001/Weather    >Current Observations</a><br />");
                              client.fastrprintln(F("<br />\r\n"));
                              client.println("<body />\r\n");
                              client.println("<br />\r\n");
                              client.println("</html>\r\n");

                              fileDownload = 0;

                         }
                         else if((strncmp(path, "/LOG", 4) == 0) ||  (strcmp(path, "/ACCESS.TXT") == 0) || (strcmp(path, "/DIFFER.TXT") == 0)|| (strcmp(path, "/SERVER.TXT") == 0) || (strcmp(path, "/README.TXT") == 0))  // Respond with the path that was accessed.
                         {

                              fileDownload = 1;   //File download has started; used to stop logFile from logging during download

                              char *filename;
                              char name;
                              strcpy( MyBuffer, path );
                              filename = &MyBuffer[1];

                              if ((strncmp(path, "/SYSTEM~1", 9) == 0) || (strncmp(path, "/ACCESS", 7) == 0))
                              {

                                   client.println("HTTP/1.1 404 Not Found");
                                   client.println("Content-Type: text/html");
                                   client.println();
                                   client.println("<h2>404</h2>\r\n");
                                   delay(250);
                                   client.println("<h2>File Not Found!</h2>\r\n");

                              }
                              else if(file.isDir())
                              {

                                   client.println("HTTP/1.1 200 OK");
                                   client.println("Content-Type: text/html");
                                   client.println();
                                   client.print("<h2>Files in /");
                                   client.print(name);
                                   client.println("/:</h2>");
                                   ListFiles(client,LS_SIZE,file);
                                   file.close();

                              }
                              else
                              {

                                   client.println("HTTP/1.1 200 OK");
                                   client.println("Content-Type: text/plain");
                                   client.println("Content-Disposition: attachment");
                                   client.println("Content-Length:");
                                   client.println("Connnection: close");
                                   client.println();

                                   readFile(); 

                              }

                         }
                         // Check the action to see if it was a GET request.
                         else  if(strncmp(path, "/Foxtrot", 8) == 0)
                         {
                              //Restricted file:  "ACCESS.TXT."  Attempted access from "Server Files:" results in
                              //404 File not Found!

                              char *filename = "/ACCESS.TXT";
                              strcpy(MyBuffer, filename);

                              // send a standard http response header
                              client.println("HTTP/1.1 200 OK");
                              client.println("Content-Type: text/plain");
                              client.println("Content-Disposition: attachment");
                              client.println("Content-Length:");
                              client.println();

                              fileDownload = 1;   //File download has started

                              readFile();
                         }
                         else
                         {

                              delay(1000);

                              // everything else is a 404
                              client.println("HTTP/1.1 404 Not Found");
                              client.println("Content-Type: text/html");
                              client.println();
                              client.println("<h2>404</h2>\r\n");
                              delay(250);
                              client.println("<h2>File Not Found!</h2>\r\n");
                         }
                    }
                    else
                    {
                         // Unsupported action, respond with an HTTP 405 method not allowed error.
                         client.fastrprintln(F("HTTP/1.1 405 Method Not Allowed"));
                         client.fastrprintln(F(""));
                    }
                    
               // Wait a short period to make sure the response had time to send before
               // the connection is closed (the CC3000 sends data asyncronously).

               delay(10);

               Serial.begin(115200);

               // Close the connection when done.
               client.close();
               Serial.println("Client closed");
               Serial.println("");

               Serial.end();

          }
          
          //cc3000.disconnect();   //Used to test init_network()  --Leave commented out otherwise
                    
          //  check wireless lan connective --if needed re-establish connection
          if (!cc3000.checkConnected())      // make sure still connected to wireless network
          {

               reConnect = "";
               reConnect = "Listen";

               if (!init_network())    // reconnect to WLAN
               {
                    delay(15 * 1000); // if no connection, try again later
                    return;
               }
          }
     }
}

//////////////////////////////////////////////////////////////////////
// Return true if the buffer contains an HTTP request.  Also returns the request
// path and action strings if the request was parsed.  This does not attempt to
// parse any HTTP headers because there really isn't enough memory to process
// them all.
// HTTP request looks like:
//  [method] [path] [version] \r\n
//  Header_key_1: Header_value_1 \r\n
//  ...
//  Header_key_n: Header_value_n \r\n
//  \r\n
bool parseRequest(uint8_t* buf, int bufSize, char* action, char* path)
{
     // Check if the request ends with \r\n to signal end of first line.
     if (bufSize < 2)
          return false;

     if (buf[bufSize-2] == '\r' && buf[bufSize-1] == '\n')
     {
          parseFirstLine((char*)buf, action, path);
          return true;
     }
     return false;
}


/////////////////////////////////////////////////////////
// Parse the action and path from the first line of an HTTP request.
void parseFirstLine(char* line, char* action, char* path)
{
     // Parse first word up to whitespace as action.
     char* lineaction = strtok(line, " ");

     if (lineaction != NULL)

          strncpy(action, lineaction, MAX_ACTION);
     // Parse second word up to whitespace as path.
     char* linepath = strtok(NULL, " ");

     if (linepath != NULL)

          strncpy(path, linepath, MAX_PATH);
}

///////////////
void readFile()
{

     Serial.begin(115200);
     
     Adafruit_CC3000_ClientRef client = httpServer.available();

     // Open file for Reading.
     SdFile webFile;

     webFile.open(&root, &MyBuffer[1], O_READ);
     if (!webFile.isOpen()) error("readFile");

     bool dload_Cancel = false;

     do   // @ adafruit_support_rick's do-while loop 
     {

          int count = 0;
          char buffers[BUFSIZE];
          bool done = false;

          while ((!done) && (count < BUFSIZE) && (webFile.available()))
          {
               char c = webFile.read();
               if (0 > c)
                    done = true;
               else
                    buffers[count++] = c;
               delayMicroseconds(1000);
          }

          if (count)
          {
               if (client.connected())
               {
                    
                    int countLoop;
                    
                    client.write(buffers, count);
                    countLoop++;
                                    
                    if(countLoop == 999)
                    {           
                        
                         watchDog();   //Prevent WDT from causing Arduino RESET during download
                         
                         countLoop = 0;
                                                  
                    }           
                    
               }
               else               
               {
                    dload_Cancel = true;
                    break;
               }
          }

     }
     while (webFile.available());

     webFile.close();

     fileDownload = 0;  //File download has finished; allow logging since download has completed

     delay(500);

     MyBuffer[0] = '\0';


     Serial.end();
}
///////////////
void watchDog()    //Keep "alive" for SwitchDoc Labs, "Dual WatchDog Timer"
{

     //Sends pulse  every minute to keep external "SwitchDoc Labs, Dual Watchdog Timer" from resetting Arduino Mega

     pinMode(RESET_WATCHDOG1, OUTPUT);
     delay(200);
     pinMode(RESET_WATCHDOG1, INPUT);

     /*
     Serial.begin(115200);
     Serial.println("\nWDT Pulsed");
     Serial.end();
     */
     
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
//  DS1307 Date and Time Stamping  Orginal function by
//  Bernhard    http://www.backyardaquaponics.com/forum/viewtopic.php?f=50&t=15687
//  Modified by Tech500 to use RTCTimedEvent library
//////////////////////////////////////////////////////////////////////////////////////////////////////////////
String getDateTime()
{

     RTCTimedEvent.readRTC();
     int temp;

     temp = (RTCTimedEvent.time.month);
     if (temp < 10)
     {
          SMonth = ("0" + (String)temp);
     }
     else
     {
          SMonth = (String)temp;
     }

     temp = (RTCTimedEvent.time.day);
     if (temp < 10)
     {
          SDay = ("0" + (String)temp);
     }
     else
     {
          SDay = (String)temp;
     }

     SYear = (String)(RTCTimedEvent.time.year);

     temp = (RTCTimedEvent.time.hour);
     if (temp < 10)
     {
          SHour = ("0" + (String)temp);
     }
     else
     {
          SHour = (String)temp;
     }

     temp = (RTCTimedEvent.time.minute);
     if (temp < 10)
     {
          SMin = ("0" + (String)temp);
     }
     else
     {
          SMin = (String)temp;
     }

     temp = (RTCTimedEvent.time.second);
     if (temp < 10)
     {
          SSec = ("0" + (String)temp);
     }
     else
     {
          SSec = (String)temp;
     }

     dtStamp = SMonth + '/' + SDay + '/' + SYear + " , " + SHour + ':' + SMin + ':' + SSec;

     return(dtStamp);
}

////////////////
float getDHT22()
{

     // Reading temperature or humidity takes about 250 milliseconds!
     // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
     h = dht.readHumidity();
     // Read temperature as Celsius
     t = dht.readTemperature();
     // Read temperature as Fahrenheit
     f = dht.readTemperature(true);

     // Wait a few seconds between measurements.
     delay(500);

     // Check if any reads failed and exit early (to try again).
     if (isnan(h) || isnan(t) || isnan(f))
     {
          Serial.println("Failed to read from DHT sensor!");
     }

     // Compute heat index
     // Must send in temp in Fahrenheit!
     hi = dht.computeHeatIndex(f, h);

     double VaporPressureValue = h * 0.01 * 6.112 * exp((17.62 * t) / (t + 243.12));
     double Numerator =243.12 * log(VaporPressureValue) - 440.1;
     double Denominator = 19.43 - (log(VaporPressureValue));
     dewPoint = Numerator / Denominator;
}
////////////////
void getBMP085()   //Get Barometric pressure readings
{

     dps.getTemperature(&Temperature);
     dps.getPressure(&Pressure);
     dps.getAltitude(&Altitude);

     currentPressure = (Pressure *  0.000295333727);   //convert to inches mercury
     milliBars = ((Pressure) * .01);   //Convert to millibars
}

////////////////////////
float updateDifference()  //Pressure difference for fifthteen minute interval
{

     //Function to find difference in Barometric Pressure
     //First loop pass pastPressure and currentPressure are equal resulting in an incorrect difference result.  Output "...Processing"
     //Future loop passes difference results are correct

     difference = currentPressure - pastPressure;  //This will be pressure from this pass thru loop, pressure1 will be new pressure reading next loop pass
     if (difference == currentPressure)
     {
          difference = 0;
     }

     return(difference);  //Barometric pressure change in inches of Mecury

}

////////////////////////////////
void beep(unsigned char delayms)
{

     // wait for a delayms ms
     digitalWrite(sonalertPin, HIGH);       // High turns on Sonalert tone
     delay(3000);
     digitalWrite(sonalertPin, LOW);  //Low turns of Sonalert tone

}

/////////////
void newDay()   //Collect Data for twenty-four hours; then start a new day
{

     //Do file maintence on 7th day of week at appointed time from RTC.  Assign new name to "log.txt."  Create new "log.txt."
     if ((RTCTimedEvent.time.dayOfWeek) == 7)
     {
          fileStore();
     }

     //id = 1;   //Reset id for start of new day
     //Write logFile Header

     // Open file from appended writing
     SdFile logFile("log.txt", O_WRITE | O_CREAT | O_APPEND);
     if (!logFile.isOpen()) error("log");
     {
          Serial.begin(115200);

          delay(1000);
          logFile.println(", , , , , ,"); //Just a leading blank line, in case there was previous data
          logFile.println("Date, Time, Humidity, Dew Point, Temperature, Heat Index, in. Hg., Difference, millibars, atm, Altitude");
          logFile.close();
          Serial.println("");
          Serial.println("Date, Time, Humidity, Dew Point, Temperature, Heat Index, in. Hg., Difference, millibars, atm, Altitude");
          Serial.flush();
          Serial.end();
     }
}

////////////////
void fileStore()   //If 7th day of week, rename "log.txt" to ("log" + month + day + ".txt") and create new, empty "log.txt"
{

     // create a file and write one line to the file
     SdFile logFile("log.txt", O_WRITE | O_CREAT );

     if (!logFile.isOpen())
     {
          error("log.txt --new -open");
     }

     // rename the file log.txt
     // sd.vwd() is the volume working directory, root.

     logFileName = "";
     logFileName = "log";
     logFileName += (RTCTimedEvent.time.month);
     logFileName += (RTCTimedEvent.time.day);
     logFileName += ".txt";
     //Serial.println(logFileName.c_str());

     if(sd.exists("log.txt"))
     {
          logFile.rename(sd.vwd(), logFileName.c_str());
          logFile.close();
     }
     else
     {
          exit;
     }

     // create a new "log.txt" file for appended writing
     logFile.open("log.txt", O_WRITE | O_CREAT | O_APPEND);
     logFile.println("");
     logFile.close();

     Serial.begin(115200);

     Serial.println("");
     Serial.println("New LOG.TXT created");

     // list files
     cout << pstr("------") << endl;
     sd.ls(LS_R);

     Serial.flush();
     Serial.end();

}

/////////////////////
int8_t init_network()   //Guard connection  --restart wireless connection if connection is lost
{

     Serial.begin(115200);

     Adafruit_CC3000_Client  client = cc3000.connectTCP(ip, LISTEN_PORT);   //Guard --  re-initalize WLAN connectivity

     Serial.println("Reconnecting to WLAN:  " + dtStamp +" Called from:  " + reConnect);
     cc3000.reboot();

     // create a file and write one line to the file
     SdFile serverFile;
     serverFile.open("Server.txt", O_WRITE | O_CREAT | O_APPEND);
     if (!serverFile.isOpen()) error("Server");

     if (serverFile.isOpen())
     {
          getDateTime();

          serverFile.print("Reconnecting to Wireless LAN:  " + dtStamp + "  ");
          serverFile.println(reConnect);  //Log where "init_network" was called from in the Sketch
          
     }
     else
     {
          Serial.println("Couldn't open server file");
     }

     // Set up the CC3000, connect to the access point, and get an IP address.

     if (!cc3000.begin() )

          while (! Serial);

     delay(500);

     /* Attempt to connect to an access point */
     char *ssid = WLAN_SSID;             /* Max 32 chars */
     Serial.print(F("\nAttempting to connect to "));
     Serial.println(ssid);

     //Keep SwitchDoc Labs, WDT alive
     watchDog();

     if (!cc3000.connectToAP(WLAN_SSID, WLAN_PASS, WLAN_SECURITY))
     {
          Serial.println(F("Failed!"));
          while(1);
          return -1;
     }
     
     delay(15 * 1000);

     Serial.println(F("Connecting to Wireless Network!"));
     Serial.println(F("Request DHCP..."));

     while (!cc3000.checkDHCP())
     {
          delay(100); 
     }

     // Display the IP address DNS, Gateway, etc.
     while (! displayConnectionDetails())
     {
          delay(1000);
     }

     httpServer.begin();
     
     //Keep SwitchDoc Labs, WDT alive
     watchDog();
     
     getDateTime();

     Serial.println("");
     Serial.println("Server Started --init Network");
     Serial.println("Connected to Wireless LAN:  " + dtStamp);
     serverFile.println("Connected to Wireless LAN: " + dtStamp + "  ");
     Serial.println("");
     Serial.println(F("Listening for connections..."));
     Serial.println("");

     Serial.end();
     
     serverFile.close();
     
     loop();

}

///////////////////////////////////
// Tries to read the IP address and other connection details
bool displayConnectionDetails()
{
     uint32_t ipAddress, netmask, gateway, dhcpserv, dnsserv;

     if(!cc3000.getIPAddress(&ipAddress, &netmask, &gateway, &dhcpserv, &dnsserv))
     {
          Serial.println(F("Unable to retrieve the IP Address!\r\n"));
          return false;
     }
     else
     {
          Serial.print(F("\nIP Addr: "));
          cc3000.printIPdotsRev(ipAddress);
          Serial.print(F("\nNetmask: "));
          cc3000.printIPdotsRev(netmask);
          Serial.print(F("\nGateway: "));
          cc3000.printIPdotsRev(gateway);
          Serial.print(F("\nDHCPsrv: "));
          cc3000.printIPdotsRev(dhcpserv);
          Serial.print(F("\nDNSserv: "));
          cc3000.printIPdotsRev(dnsserv);
          Serial.println();
          return true;
     }
}





