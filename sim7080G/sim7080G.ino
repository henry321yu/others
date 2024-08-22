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

#include <SoftwareSerial.h> //sim7080(mcu's rx、hc-12's tx,   mcu's tx、hc-12's rx) 
SoftwareSerial sim7080(7, 8); int setpin = 9; //old box
void setup()
{
  // Open serial communications and wait for port to open:
  Serial.begin(9600);


  Serial.println(F("sim7080g set"));
  pinMode(setpin, OUTPUT);   
  digitalWrite(setpin, HIGH); delay(2000);  
  digitalWrite(setpin, LOW); delay(2000); 
  sim7080.begin(9600);
  
  Serial.println("done initial");
}

void loop() // run over and over
{

  while (sim7080.available()) {
    Serial.write(sim7080.read());
    Serial.println("while (sim7080.available()) {");
  }
  while (Serial.available()) {
    sim7080.write(Serial.read());
    Serial.println("while (Serial.available()) {");
  }
  delay(1);
}
