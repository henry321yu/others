#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
#include <printf.h>

const int pinCE = 2;
const int pinCSN = 15;
byte gotByte = 0;

RF24 wirelessSPI(pinCE, pinCSN);
const uint64_t pAddress = 0xB00B1E5000LL;

void setup()
{
  Serial.begin(115200);
  wirelessSPI.begin();
  wirelessSPI.setAutoAck(1);
  wirelessSPI.enableAckPayload();
  wirelessSPI.setRetries(5, 15);
  wirelessSPI.openReadingPipe(1, pAddress);
  wirelessSPI.startListening();
  wirelessSPI.printDetails();
}

void loop()
{
  while (wirelessSPI.available()) {
    wirelessSPI.read( &gotByte, sizeof(gotByte) );
    Serial.print("Recieved packet number: ");
    Serial.println(gotByte);
    Serial.print("Size: ");
    Serial.println(sizeof(gotByte));

    wirelessSPI.printDetails();
  }

  delay(100);
}
