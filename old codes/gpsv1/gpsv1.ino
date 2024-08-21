#include <SoftwareSerial.h> 

SoftwareSerial GPS(2,3);//rx,tx 
//#define GPS Serial

void setup()
{
  Serial.begin(9600);
  GPS.begin(9600);
  Serial.println("go") ;
}
void loop()
{
  if(GPS.available()>0)
  {
      Serial.println("something") ;
  }
  else{
      Serial.println("nothing") ;
      delay(1000);
}
}
