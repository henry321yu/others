/*
  Software serial multple serial test

  Receives from the hardware serial, sends to software serial.
  Receives from software serial, sends to hardware serial.

  The circuit:
   RX is digital pin 7 (connect to TX of other device)
   TX is digital pin 8 (connect to RX of other device)

  created back in the mists of time
  modified 25 May 2012
  by Tom Igoe
  based on Mikal Hart's example

  This example code is in the public domain.
*/

#include <SoftwareSerial.h> //HC12(mcu's rx、hc-12's tx,   mcu's tx、hc-12's rx) 
//SoftwareSerial HC12(7, 8); int setpin = 9; //old box
SoftwareSerial HC12(21, 20); int setpin = 22; //small box
//SoftwareSerial HC12(13, 12);int setpin = 11; //pico
void setup()
{
  // Open serial communications and wait for port to open:
  Serial.begin(115200);


  Serial.println(F("HC12.reset"));
  pinMode(setpin, OUTPUT); digitalWrite(setpin, LOW); // for reset
  HC12.begin(9600);
  delay(100);
  HC12.print("AT+B115200");
  delay(100);
  Serial.println(F("HC12.begin and set"));
  HC12.begin(115200);
  delay(100);
  HC12.print("AT+B115200");
  delay(100);
  HC12.print("AT+C127"); //127 for imu //117 for mag sensor //107 for gps
  delay(100);
  HC12.print("AT+P8");
  delay(100);
  digitalWrite(setpin, HIGH);
  Serial.println(F("HC12.set"));
  while (HC12.available()) {
    Serial.write(HC12.read());
  }
  Serial.println("done initial");
  HC12.println("done initialize");
}

void loop() // run over and over
{

  while (HC12.available()) {
    Serial.write(HC12.read());
  }
  while (Serial.available()) {
    HC12.write(Serial.read());
  }
  delay(1);
  /*
    Serial.println(millis());
    HC12.println(millis());
    delay(100);*/
}
