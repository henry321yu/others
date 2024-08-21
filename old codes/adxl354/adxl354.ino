// these constants describe the pins. They won't change:
const int xpin = A2;                  // x-axis of the accelerometer
const int ypin = A3;                  // y-axis
const int zpin = A4;                  // z-axis (only on 3-axis models)
double x,y,z,adc;

void setup() {
  // initialize the serial communications:
  Serial.begin(115200);
  analogReference(INTERNAL2V56);
  
  /*(xpin, INPUT); 
  pinMode(ypin, INPUT); 
  pinMode(zpin, INPUT);*/
}

void loop() {
  //analogReadResolution(16);
  adc=2.4775;//4.32;nano 4.497mega;
  x=analogRead(xpin);
  y=analogRead(ypin);
  z=analogRead(zpin);
  /*Serial.print(x,5);
  Serial.print("\t");
  Serial.print(y,5);
  Serial.print("\t");
  Serial.print(z,5);
  Serial.print("\t");*/
  
  x=(x*adc/1023-0.9)/0.4;
  y=(y*adc/1023-0.9)/0.4;
  z=(z*adc/1023-0.9)/0.4;

  Serial.print(x,5);
  Serial.print("\t");
  Serial.print(y,5);
  Serial.print("\t");
  Serial.print(z,5);
  Serial.println("\t");
  //
  delay(5);
}
