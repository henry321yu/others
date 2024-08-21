// Armuino - Arduino DS3231 Real Time Clock Module - Data Logging
// Sensor: DHT11 (temperature & humidity)
//
// DS3231 Library by Henning Karlsen (www.rinkydinkelectronics.com)
//
// Video Demo: 

#include <Wire.h>
#include <LCD.h>
#include <LiquidCrystal_I2C.h>
#include <DS3231.h>

#include <SD.h>
#include <SPI.h>
#include "SdFat.h"

#include <dht.h>

#define I2C_ADDR 0x27 // Put your I2C address here
#define Rs_pin 0
#define Rw_pin 1
#define En_pin 2
#define BACKLIGHT_PIN 3
#define D4_pin 4
#define D5_pin 5
#define D6_pin 6
#define D7_pin 7

#define lcd_W 16
#define lcd_H 2

#define dht_apin A0 // Analog Pin sensor is connected to
 
dht DHT;    // Init DHT object

LiquidCrystal_I2C lcd(I2C_ADDR, En_pin, Rw_pin, Rs_pin, D4_pin, D5_pin, D6_pin, D7_pin);
DS3231  rtc(SDA, SCL);

const int pinCS = 10; // Pin 10 - on Arduino Uno; Pin 53 - on Arduino Mega

void setup()
{ 
  Serial.begin(9600);
  
  Serial.print("Initializing SD card...");
  pinMode(pinCS, OUTPUT);
  
  if (!SD.begin(pinCS)) {  // check if SD card is present
    Serial.println("No SD Card present in module");
    return;
  }
  Serial.println("SD Card Ready");
  
  rtc.begin(); // Initialize the rtc object

  lcd.setBacklightPin(BACKLIGHT_PIN,POSITIVE); // LCD Backlight ON
  lcd.setBacklight(HIGH);
   
  lcd.home (); // Go home on LCD
  lcd.begin (lcd_W,lcd_H); //  <<----- LCD (chars x lines) lcd_W x lcd_H
}

void loop()
{
  File dataFile = SD.open("dht-data.txt", FILE_WRITE);  // Open or Create file 

  // Read temperature and humidity
  DHT.read11(dht_apin);
  float h = DHT.humidity;
  float t = DHT.temperature;
  
  Serial.print("Date: ");
  Serial.print(rtc.getDateStr());
  Serial.print(", Time: ");
  Serial.print(rtc.getTimeStr());
  Serial.print(", RTC Temperature: ");
  Serial.println(rtc.getTemp());

  Serial.print("Ambient humidity = ");
  Serial.print(h);
  Serial.print(" %  ");
  Serial.print("temperature = ");
  Serial.print(t); 
  Serial.println(" C  ");
/*     
  Serial.print("Current RTC Temperature: ");
  Serial.println(rtc.getTemp());
*/
  lcd.setCursor(0,0);
  lcd.print(rtc.getDateStr(FORMAT_SHORT));
  lcd.print(" - ");
  lcd.print(rtc.getTimeStr(FORMAT_SHORT));
  
  lcd.setCursor(0,1);
  lcd.print("Temper-re: ");
  lcd.print(t);
  delay(3000);

  lcd.setCursor(0,1);
  lcd.print("Humidity: ");
  lcd.print(h);

  if (dataFile)
  {  // Check if file exist on SD Card
     dataFile.print("Date: ");
     dataFile.print(rtc.getDateStr());
     dataFile.print(", Time: ");
     dataFile.print(rtc.getTimeStr());
     dataFile.print(", Temperature: ");
     dataFile.print(t);
     dataFile.print(", Humidity: ");
     dataFile.println(h);
     
     dataFile.close();  // Close file
  }  
  else {
    Serial.println("Error opening the file!"); // if file not on SD Card
  }
  
  delay(3000); 
}
