#include <SD.h>
#include <SPI.h>
#include <DS3231.h>
File sdcard_file;
DS3231  rtc(SDA, SCL);
int CS_pin = 10; // Pin 10 on Arduino Uno
const int sensor_pin = A0;
float temp;  
float output;

void setup() {
  Serial.begin(9600);
  pinMode(sensor_pin,INPUT);
  pinMode(CS_pin, OUTPUT);
  rtc.begin(); 
  // SD Card Initialization
  if (SD.begin())
  {
    Serial.println("SD card is ready to use.");
  } else
  {
    Serial.println("SD card initialization failed");
    return;
  }
  
  Serial.print("Date  ");   
  Serial.print("      ");
  Serial.print("   Time  ");
  Serial.print("     ");
  Serial.print("   Temp   ");
  Serial.println("     ");
  sdcard_file = SD.open("data.txt", FILE_WRITE);
  if (sdcard_file) { 
    sdcard_file.print("Date  ");   
    sdcard_file.print("      ");
    sdcard_file.print("   Time  ");
    sdcard_file.print("     ");
    sdcard_file.print("   Temp   ");
    sdcard_file.println("     ");
    sdcard_file.close(); // close the file
  }
  // if the file didn't open, print an error:
  else {
    Serial.println("error opening test.txt");
  }
}

void loop() {
  output = analogRead(sensor_pin);
  temp =(output*500)/1023;
  Serial.print(rtc.getDateStr());
  Serial.print("     ");
  Serial.print(rtc.getTimeStr());
  Serial.print("      ");
  Serial.println(temp);
 
  sdcard_file = SD.open("data.txt", FILE_WRITE);
  if (sdcard_file) {    
    sdcard_file.print(rtc.getTimeStr());
    sdcard_file.print("     ");   
    sdcard_file.print(rtc.getTimeStr());
    sdcard_file.print("     ");
    sdcard_file.println(temp);
    sdcard_file.close(); // close the file
  }
  // if the file didn't open, print an error:
  else {
    Serial.println("error opening test.txt");
  }
  delay(3000);
}
