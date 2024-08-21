#include <SPI.h>
#include "RF24.h"

RF24 rf24(8, 10, 4000000); // CE腳, CSN腳

const byte addr[] = "1Node";
const byte pipe = 1;  // 指定通道編號
unsigned long i = 0;
double f;
int LED = 3;

void setup() {
  pinMode(LED, OUTPUT);
  Serial.begin(115200);
  delay(1000);
  rf24.begin();
  delay(1000);
  rf24.setChannel(83);  // 設定頻道編號
  delay(1000);
  rf24.setPALevel(RF24_PA_MIN);
  delay(1000);
  rf24.setDataRate(RF24_250KBPS);
  delay(1000);
  rf24.openReadingPipe(pipe, addr);  // 開啟通道和位址
  delay(1000);
  rf24.startListening();  // 開始監聽無線廣播
  delay(1000);
  rf24.printPrettyDetails();
  delay(1000);
  Serial.println("nRF24L01 ready!");
}

void loop() {
  if (rf24.available(&pipe)) {
    digitalWrite(LED, HIGH);
    char msg[32] = "";
    rf24.read(&msg, sizeof(msg));
    Serial.print(msg); // 顯示訊息內容
    //Serial.println(sizeof(msg));

    ff(msg);
  }
  digitalWrite(LED, LOW);
}

void ff(char msg[32]) {
  for (int j; j < sizeof(msg); j++) {
    if (msg[j] == '\n') {
      double t = millis() / 1000;
      f = i / t;
      Serial.print(f);
      Serial.println(" hz");
      i++;
    }
  }
}
