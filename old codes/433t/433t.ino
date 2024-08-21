#include <RH_ASK.h>
#include <SPI.h>

RH_ASK driver(2000, 4, 2, 5);

void setup()
{
  Serial.begin(115200);
  if (!driver.init()) {
    Serial.println("init failed");
  }

  Serial.println("inited");
}
void loop() {
  Serial.println("transmited");
  //const char * msg = "hello";
  //const char * msg = "-36.54785, -15.90576, 9.61304, -15.90576, 9.61304, -15.90576, -15.90576, 9.61304, -15.90576";
  const char * msg = "36000.001,-360.001,-360.001,-360.001,-36.001,-36.001,-36.001";
  //const char * msg = "36000.001";

  Serial.println(msg);
  Serial.println(strlen(msg));
  driver.send((uint8_t*)msg, strlen(msg));
  driver.waitPacketSent();
  //delay(200);

  /*double t = millis();
    t = t / 1000;
    String flydata = String(t, 3) + ", " + String(t + 1, 7) + ", " + String(t + 2, 7) + ", " + String(t + 3, 7) + ", " + String(t + 4, 6) + ", " + String(t + 5, 6) + ", " + String(t + 6, 6) + ", " + String(t + 7, 5) + ", " + String(t + 8, 5) + ", " + String(t + 9, 5);
    //mySerial.println(flydata);  //左方板向PC傳送字串
    //Serial.println(flydata);

    char tt=t;
    driver.send(tt, strlen(tt));
    driver.waitPacketSent();
    delay(200);
    Serial.println(tt);*/
}
