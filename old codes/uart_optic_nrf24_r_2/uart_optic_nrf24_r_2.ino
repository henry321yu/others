#include <SoftwareSerial.h>
#include <SPI.h>
#include "printf.h"
#include "RF24.h"

SoftwareSerial mySerial(0, 1); //建立軟體串列埠腳位 (RX, TX)
RF24 radio(8, 10);  // using pin 7 for the CE pin, and pin 8 for the CSN pin

// Let these addresses be used for the pair
const byte addr[] = "1Node";

int LED = 23;
String temp;

String logdata = "";
float data[10];
int i = 0; int j = 0; int k = 0;
int tag[20];
char buf[200];

unsigned long ff = 0;
double t, f;
byte stuck = 1;

void setup() {
  pinMode(LED, OUTPUT);
  Serial.begin(115200);        //設定硬體串列埠速率
  mySerial.begin(115200);   //設定軟體串列埠速率

  if (!radio.begin()) {
    Serial.println(F("radio hardware is not responding!!"));
    while (1) {}  // hold in infinite loop
  }
  radio.setPALevel(RF24_PA_LOW);  // RF24_PA_MAX is default.
  //radio.setPayloadSize(sizeof(payload));  // float datatype occupies 4 bytes
  radio.openWritingPipe(addr);  // always uses pipe 0
  radio.openReadingPipe(1, addr);  // using pipe 1
  radio.stopListening();  // put radio in TX mode

  //For debugging info
  //printf_begin();             // needed only once for printing details
  //radio.printDetails();       // (smaller) function that prints raw register values
  //radio.printPrettyDetails(); // (larger) function that prints human readable data
  pinMode(LED, OUTPUT);

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
      //Serial.println(sizeof(data[0]));


      t = millis() / 1000;
      if (t > 5) {
        /////////////////////////////

        char msgg[logdata.length()];
        logdata.toCharArray(msgg, logdata.length() + 1); //新增char 版

        //String tmsg[10];
        char ttmsg[32] = "";
        bool report;
        String tttmsg = "";
        unsigned long start_timer;
        unsigned long end_timer;
        String tmsg[(logdata.length() / 32) + 1];
        byte g = 0; byte h = 1;

        for (int u = 0; u < logdata.length(); u++) {
          tmsg[g] += msgg[u];
          h++;
          if (h % 31 == 0) { //每32 char分割
            g++;
          }
          delayMicroseconds(1);
        }

        Serial.print(F("transmit "));  // payload was delivered
        Serial.print(sizeof(msgg));  // payload was delivered
        Serial.println(F(" chars"));  // payload was delivered
        for (g = 0; g < (logdata.length() / 32) + 1; g++) { //每32 char分割發送
          //Serial.print(tmsg[j]);
          tmsg[g].toCharArray(ttmsg, tmsg[g].length() + 1); //新增char 版
          start_timer = micros();                // start the timer
          report = radio.write(&ttmsg, sizeof(ttmsg)); // 傳送資料
          end_timer = micros();                  // end the timer
          tttmsg += tmsg[g];
          Serial.print(ttmsg);
        }
        char wtf = '\n';
        //radio.write(&wtf, sizeof(wtf)); // 傳送資料
        Serial.print(wtf);

        Serial.print(F("separat "));
        Serial.print(tttmsg.length());
        Serial.println(F(" strings"));
        Serial.println(tttmsg);

        Serial.print(F("original "));
        Serial.print(logdata.length());
        Serial.println(F(" strings"));
        Serial.println(logdata);


        if (report) {
          Serial.print(F("Transmission successful! "));  // payload was delivered
          Serial.print(F("Time to transmit = "));
          Serial.print(end_timer - start_timer);  // print the timer result
          Serial.print(F(" us ,complete by "));
          Serial.print(g);
          Serial.println(F(" times"));
          digitalWrite(LED, HIGH);
        } else {
          Serial.println(F("Transmission failed or timed out"));  // payload was not delivered
          digitalWrite(LED, LOW);
        }
        /////////////////////////////

        f = ff / t;
        ff++;
        Serial.print(f);  // if f down to 3hz capacitor uncontact
        Serial.println(" hz");

      }



      logdata = "";
      i = 0; j = 0; k = 0;
    }
  }//while (mySerial.available()) {

  delay(10);
}
