#include <RH_ASK.h>
#include <SPI.h>

RH_ASK driver(2000, 2, 4, 5);

void setup()
{
  Serial.begin(115200);
  if (!driver.init()) {
    Serial.println("init failed");
  }

  Serial.println("inited");
}
void loop() {
  uint8_t buf[RH_ASK_MAX_MESSAGE_LEN];
  uint8_t buflen = sizeof(buf);

  if (driver.recv(buf, &buflen)) {
    driver.printBuffer("Got:", buf, buflen);
    String rcv;
    for (int i = 0; i < buflen; i++) {
      rcv += (char)buf[i];
    }
    Serial.println(rcv);
  }
}
