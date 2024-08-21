/*
  DigitalReadSerial

  Reads a digital input on pin 2, prints the result to the Serial Monitor

  This example code is in the public domain.

  http://www.arduino.cc/en/Tutorial/DigitalReadSerial
*/

// digital pin 2 has a pushbutton attached to it. Give it a name:
int Dread = 0,D = 52;
int a1 = A0,a2=A1,a3=A4;
String Aread;

// the setup routine runs once when you press reset:
void setup() {
  // initialize serial communication at 9600 bits per second:
  Serial.begin(115200);
  // make the pushbutton's pin an input:
  pinMode(D, INPUT);
}

// the loop routine runs over and over again forever:
void loop() {
  Dread = digitalRead(D);
  Aread = analogRead(a1);
  Aread += ", ";
  Aread += analogRead(a2);
  //Aread += ", ";
  //Aread += analogRead(a3);
  //Serial.print("D =");
  //Serial.println(Dread);
  //Serial.print("A =");
  Serial.println(Aread);
  delay(5);
}
