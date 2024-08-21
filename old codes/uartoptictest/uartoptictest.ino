#include <SoftwareSerial.h>
SoftwareSerial mySerial(7, 8); //建立軟體串列埠腳位 (RX, TX)
int LED = 13;
String flydata;

void setup() {
  pinMode(LED, OUTPUT);
  Serial.begin(115200);        //設定硬體串列埠速率
  mySerial.begin(115200);   //設定軟體串列埠速率
}

void loop() {
  double t=millis();
  t=t/1000;
  flydata = String(t, 3) + "," + String(t+1, 7) + "," + String(t+2, 7) + "," + String(t+3, 7) + "," + String(t+4, 6) + "," + String(t+5, 6) + "," + String(t+6, 6) + "," + String(t+7, 5) + "," + String(t+8, 5) + "," + String(t+9, 5);
  mySerial.println(flydata);  //左方板向PC傳送字串
  Serial.println(flydata);
  digitalWrite(LED, HIGH);
  delay(10);
  digitalWrite(LED, LOW);
  delay(10);
}
