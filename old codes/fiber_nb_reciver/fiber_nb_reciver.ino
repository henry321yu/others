#include <SoftwareSerial.h>
#include <SPI.h>
SoftwareSerial mySerial(7, 8); //建立軟體串列埠腳位 (RX, TX)
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

    char inChar = mySerial.read();
    logdata += (char)inChar;
    Serial.print((char)inChar);
    }
}
