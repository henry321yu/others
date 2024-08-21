#include <SoftwareSerial.h>
#include <SPI.h>
SoftwareSerial mySerial(0, 1); //建立軟體串列埠腳位 (RX, TX)
int LED = 23;
String temp;

String logdata = "";
float data[10];
int i = 0; int j = 0; int k = 0;
int tag[20];
char buf[200];
void setup() {
  pinMode(LED, OUTPUT);
  Serial.begin(115200);        //設定硬體串列埠速率
  mySerial.begin(115200);   //設定軟體串列埠速率
  //while (!Serial) {    ;  }
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
          tag[j + 1] = i + 1; //跳過", "
          j++;
        }
      }
      for (k = 0; k < j; k++) {//將字串以,分割並分別存入data
        temp = logdata.substring(tag[k], tag[k + 1]);
        data[k] = temp.toFloat(); //限制5位數 !!!
        //Serial.println(temp);
        Serial.println(data[k], 7);
        if (k == j - 1) {
          String temp = logdata.substring(tag[k + 1]);
          data[k + 1] = temp.toFloat();
          Serial.println(data[k + 1], 7);
        }
      }


      int dataa = data[0];
      logdata = "";
      i = 0; j = 0; k = 0;
      //Serial.println(sizeof(data[0]));

      /*const char * msg = "hello";
        //temp.toCharArray(wtf, temp.length());
        driver.send((uint8_t*)msg, strlen(msg));
        driver.waitPacketSent();
        Serial.println (msg) ;*/
    }

    digitalWrite(LED, HIGH);
  }
  
  delay(1);
  digitalWrite(LED, LOW);
  //Serial.println("  loop  ");
}
