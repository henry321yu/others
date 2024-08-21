#include <SoftwareSerial.h>
#include <SPI.h>
#include "RF24.h"

SoftwareSerial mySerial(0, 1); //建立軟體串列埠腳位 (RX, TX)
int LED = 13;
String temp;

String logdata = "";
float data[10];
int i = 0; int j = 0; int k = 0;
int tag[20];
char buf[200];

RF24 rf24(7, 8); // CE腳,CSN腳
const byte addr[] = "1Node";
unsigned long ff = 0;
double t, f;

void setup() {
  pinMode(LED, OUTPUT);
  Serial.begin(115200);        //設定硬體串列埠速率
  mySerial.begin(115200);   //設定軟體串列埠速率

  rf24.begin();
  rf24.setChannel(83);       // 設定頻道編號
  rf24.openWritingPipe(addr); // 設定通道位址
  rf24.setPALevel(RF24_PA_MIN);   // 設定廣播功率
  rf24.setDataRate(RF24_250KBPS); // 設定傳輸速率
  rf24.stopListening();       // 停止偵聽；設定成發射模式

  Serial.println("done int");
}

void loop() // run over and over
{
  while (mySerial.available()) {
    //Serial.write(mySerial.read());

    char inChar = mySerial.read();
    logdata += (char)inChar;
    if (inChar == '\n') {
      Serial.print("logdata ");
      Serial.print(logdata);

      logdata.toCharArray(buf, logdata.length());
      for (i = 0; i < logdata.length(); i++) {
        if (buf[i] == ',') {
          tag[0] = 0;
          tag[j + 1] = i + 2; //跳過", "
          j++;
        }
      }
      for (k = 0; k < j; k++) {//將字串以,分割並分別存入data
        temp = logdata.substring(tag[k], tag[k + 1]);
        data[k] = temp.toFloat(); //限制5位數 !!!
        //Serial.println(temp);
        //Serial.println(data[k], 7);
        if (k == j - 1) {
          String temp = logdata.substring(tag[k + 1]);
          data[k + 1] = temp.toFloat();
          //Serial.println(data[k + 1], 7);
        }
      }


      int dataa = data[0];
      //Serial.println(sizeof(data[0]));


      t = millis() / 1000;
      if (t > 5) {
        /////////////////////////////

        char msgg[logdata.length()];
        logdata.toCharArray(msgg, logdata.length() + 1); //新增char 版

        char ttmsg[32] = "";
        String tmsg[(logdata.length() / 32) + 1];
        byte g = 0; byte h = 1;
        for (int u = 0; u < logdata.length(); u++) {
          tmsg[g] += msgg[u];
          h++;
          if (h % 31 == 0) { //每32 char分割
            g++;
          }
        }

        Serial.print("rftdata ");
        for (g = 0; g < (logdata.length() / 32) + 1; g++) { //每32 char分割發送
          //Serial.println(sizeof(ttmsg));
          //Serial.println(tmsg[g]);
          tmsg[g].toCharArray(ttmsg, tmsg[g].length() + 1); //新增char 版
          rf24.write(&ttmsg, sizeof(ttmsg));  // 傳送每32資料
          Serial.print(ttmsg);
          //delay(1);
        }

        String tttmsg = tmsg[0] + tmsg[1] + tmsg[2] + tmsg[3] + tmsg[4];
        //Serial.print(tttmsg);
        Serial.print("tttmsg.length() ");
        Serial.println(tttmsg.length());


        /////////////////////////////

        f = ff / t;
        ff++;
        Serial.print(f);  // if f down to 3hz capacitor uncontact
        Serial.println(" hz");
        //delayMicroseconds(1000);
      }



      logdata = "";
      i = 0; j = 0; k = 0;
    }

    digitalWrite(LED, HIGH);
  }

  delay(1);
  digitalWrite(LED, LOW);
  //Serial.println("  loop  ");

}
