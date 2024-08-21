#include <SPI.h>
#include "RF24.h"
#include "printf.h"

RF24 rf24(8, 10,4000000); // CE腳, CSN腳

const byte addr[] = "1Node";
//const char msg[] = "Happy Hacking!";
//const char msg[] = "88510.092, -0.0058095, 0.9951864, -0.0227464, -0.0011787, 0.9965051, -0.0104085,-0.234768, 0.449406, -0.009111";

unsigned long ff = 0;
double t, f;

void setup() {
  Serial.begin(115200);
  delay(1000);
  rf24.begin();  
  delay(1000);
  rf24.setChannel(83);       // 設定頻道編號
  delay(1000);
  rf24.openWritingPipe(addr); // 設定通道位址
  delay(1000);
  rf24.setPALevel(RF24_PA_MIN);   // 設定廣播功率
  delay(1000);
  rf24.setDataRate(RF24_250KBPS); // 設定傳輸速率
  delay(1000);
  rf24.stopListening();       // 停止偵聽；設定成發射模式
  delay(1000);
  rf24.printPrettyDetails(); 
  delay(1000);
}

void loop() {
  String msg = String(t / 1000, 3) + "," + String(t / 995, 5) + "," + String(t / 832, 6) + "," + String(t / 750, 7) + "," + String(t / 1100, 7) + "," + String(t / 1052, 7) + "," + String(t / 1024, 7);
  char msgg[msg.length()];
  msg.toCharArray(msgg, msg.length() + 1); //新增char 版

  String tmsg[3];
  char ttmsg[32] = "";
  byte j = 0; byte k = 1;
  for (int i = 0; i < msg.length(); i++) {
    tmsg[j] += msgg[i];
    k++;
    if (k % 31 == 0) {//每32 char分割
      j++;
    }
  }

  Serial.println("transmitting");
  /*
    Serial.println(tmsg[0]);
    Serial.println(tmsg[0].length());
    Serial.println(tmsg[1]);
    Serial.println(tmsg[1].length());
    Serial.println(tmsg[2]);
    Serial.println(tmsg[2].length());*/


  for (j = 0; j < 3; j++) { //每32 char分割發送
    //Serial.println(tmsg[j]);
    tmsg[j].toCharArray(ttmsg, tmsg[j].length() + 1); //新增char 版
    rf24.write(&ttmsg, sizeof(ttmsg));  // 傳送資料
    Serial.print(ttmsg);
  }
  char wtf = '\n';
  rf24.write(&wtf, sizeof(wtf));  // 傳送資料
  Serial.print(wtf);

  //Serial.println(msgg);
  //Serial.println(sizeof(msgg));
  String tttmsg = tmsg[0] + tmsg[1] + tmsg[2];
  Serial.println(tttmsg);
  Serial.println(tttmsg.length());
  Serial.println(msg);
  Serial.println(msg.length());
  //rf24.write(&msgg, sizeof(msgg));  // 傳送資料
  delayMicroseconds(9000);
  t = millis() / 1000;
  f = ff / t;
  ff++;
  Serial.print(f);  // if f down to 3hz capacitor uncontact
  Serial.println(" hz");
}
