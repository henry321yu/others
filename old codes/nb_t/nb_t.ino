#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>

RF24 radio(5, 6); //指定 Arduino Nano 腳位對應 nRF24L01 之 (CE, CSN)
const byte address[6] = "00001";  //節點位址為 5 bytes + \0=6 bytes

int counter=0;  //Hello 計數器
void setup() {
  Serial.begin(115200);
  radio.begin();  //初始化 nRF24L01 模組
  radio.openWritingPipe(address);  //開啟寫入管線
  radio.setPALevel(RF24_PA_MIN);   //設為低功率, 預設為 RF24_PA_MAX
  radio.stopListening();  //傳送端不需接收, 停止傾聽
  pinMode(14,OUTPUT);
  }
void loop() {
  const char text[32];  //宣告用來儲存欲傳送之字串
  sprintf(text, "Hello World %d", counter);  //將整數嵌入字串中
  Serial.println(text);
  radio.write(&text, sizeof(text));   //將字串寫入傳送緩衝器
  ++counter;
  digitalWrite(14,HIGH);
  delay(1000);
  digitalWrite(14,LOW);
  delay(1000);
  }
