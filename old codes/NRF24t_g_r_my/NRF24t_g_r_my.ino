#include <SPI.h>
#include "printf.h"
#include "RF24.h"

RF24 radio(8, 10);  // using pin 7 for the CE pin, and pin 8 for the CSN pin

// Let these addresses be used for the pair
const byte addr[] = "1Node";
float payload = 0.0;

unsigned long i = 0;
double f;
int LED = 3;

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
  radio.startListening();  // put radio in RX mode
  //radio.setDataRate(RF24_2MBPS);

  //For debugging info
  //printf_begin();             // needed only once for printing details
  //radio.printDetails();       // (smaller) function that prints raw register values
  //radio.printPrettyDetails(); // (larger) function that prints human readable data
  pinMode(LED, OUTPUT);
}

void loop() {
  // This device is a RX node
  uint8_t pipe;
  if (radio.available(&pipe)) {
    digitalWrite(LED, HIGH);
    char msg[32] = "";
    radio.read(&msg, sizeof(msg));
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
