#include <SPI.h>
#include "printf.h"
#include "RF24.h"

RF24 radio(8, 10);  // using pin 7 for the CE pin, and pin 8 for the CSN pin

// Let these addresses be used for the pair
const byte addr[] = "1Node";
float payload = 0.0;

unsigned long ff = 0;
double t, f;
int LED = 16;

void setup() {
  Serial.begin(115200);
  if (!radio.begin()) {
    Serial.println(F("radio hardware is not responding!!"));
    while (1) {}  // hold in infinite loop
  }
  radio.setPALevel(RF24_PA_LOW);  // RF24_PA_MAX is default.
  //radio.setPayloadSize(sizeof(payload));  // float datatype occupies 4 bytes
  radio.openWritingPipe(addr);  // always uses pipe 0
  radio.openReadingPipe(1, addr);  // using pipe 1
  radio.stopListening();  // put radio in TX mode
  //radio.setDataRate(RF24_2MBPS);

  //For debugging info
  //printf_begin();             // needed only once for printing details
  //radio.printDetails();       // (smaller) function that prints raw register values
  //radio.printPrettyDetails(); // (larger) function that prints human readable data
  pinMode(LED, OUTPUT);
}

void loop() {
  String msg = String(t / 1000, 3) + "," + String(t / 995, 5) + "," + String(t / 832, 6) + "," + String(t / 750, 7) + "," + String(t / 1100, 7) + "," + String(t / 1052, 7) + "," + String(t / 1024, 7) + "," + String(t / 995, 5) + "," + String(t / 832, 6) + "," + String(t / 750, 7) + "," + String(t / 1100, 7) + "," + String(t / 1052, 7) + "," + String(t / 1024, 7);
  char msgg[msg.length()];
  msg.toCharArray(msgg, msg.length() + 1); //新增char 版
  String tmsg[10];
  char ttmsg[32] = "";
  bool report;
  String tttmsg = "";
  unsigned long start_timer;
  unsigned long end_timer;
  byte j = 0; byte k = 1;

  for (int i = 0; i < msg.length(); i++) {
    tmsg[j] += msgg[i];
    k++;
    if (k % 31 == 0) {//每32 char分割
      j++;
    }
  }

  Serial.print(F("transmit "));  // payload was delivered
  Serial.print(sizeof(msgg));  // payload was delivered
  Serial.println(F(" chars"));  // payload was delivered
  for (j = 0; j < ((msg.length() / 31)) + 1; j++) { //每32 char分割發送
    //Serial.print(tmsg[j]);
    tmsg[j].toCharArray(ttmsg, tmsg[j].length() + 1); //新增char 版
    start_timer = micros();                // start the timer
    report = radio.write(&ttmsg, sizeof(ttmsg)); // 傳送資料
    end_timer = micros();                  // end the timer
    tttmsg += tmsg[j];
    Serial.print(ttmsg);
  }
  char wtf = '\n';
  radio.write(&wtf, sizeof(wtf)); // 傳送資料
  Serial.print(wtf);

  Serial.print(F("separat "));
  Serial.print(tttmsg.length());
  Serial.println(F(" strings"));
  Serial.println(tttmsg);

  Serial.print(F("original "));
  Serial.print(msg.length());
  Serial.println(F(" strings"));
  Serial.println(msg);
  t = millis() / 1000;
  f = ff / t;
  ff++;
  Serial.print(f);  // if f down to 3hz capacitor uncontact
  Serial.println(" hz");




  // This device is a TX node
  //unsigned long start_timer = micros();                // start the timer
  //bool report = radio.write(&start_timer, sizeof(start_timer));  // transmit & save the report
  //unsigned long end_timer = micros();                  // end the timer

  if (report) {
    Serial.print(F("Transmission successful! "));  // payload was delivered
    Serial.print(F("Time to transmit = "));
    Serial.print(end_timer - start_timer);  // print the timer result
    Serial.print(F(" us ,complete by "));
    Serial.print(j);
    Serial.println(F(" times"));
    digitalWrite(LED, HIGH);
  } else {
    Serial.println(F("Transmission failed or timed out"));  // payload was not delivered
    digitalWrite(LED, LOW);
  }

  //radio.printPrettyDetails(); // (larger) function that prints human readable data
  delay(5);  // slow transmissions down by 1 second
}
