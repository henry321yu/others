#include <Wire.h>

#define SERIAL_BAUD 115200
#define TCAADDR 0x70

void tcaselect(uint8_t i) {
  if (i > 7) return;

  Wire.beginTransmission(TCAADDR);
  Wire.write(1 << i);
  Wire.endTransmission();
}

void setup()
{
  Wire.begin();

  Serial.begin(SERIAL_BAUD);
  Serial.println("I2C Scanner started");
  Serial.println();
}


void loop()
{
  for (int i = 0; i < 8; i++) {
    tcaselect(i);
    uint8_t error, i2cAddress, devCount, unCount;

    Serial.print("Scanning    Number "); Serial.print(i); Serial.println("  Port...");

    devCount = 0;
    unCount = 0;
    for (i2cAddress = 1; i2cAddress < 127; i2cAddress++ )
    {
      Wire.beginTransmission(i2cAddress);
      error = Wire.endTransmission();

      if (error == 0)
      {
        Serial.print("I2C device found at 0x");
        if (i2cAddress < 16) Serial.print("0");
        Serial.println(i2cAddress, HEX);
        devCount++;
      }
      else if (error == 4)
      {
        Serial.print("Unknow error at 0x");
        if (i2cAddress < 16) Serial.print("0");
        Serial.println(i2cAddress, HEX);
        unCount++;
      }
    }

    if (devCount + unCount == 0)
      Serial.println("No I2C devices found\n");
    else {
      Serial.println();
      Serial.print(devCount);
      Serial.print(" device(s) found");
      if (unCount > 0) {
        Serial.print(", and unknown error in ");
        Serial.print(unCount);
        Serial.print(" address");
      }
      Serial.println();
    }
    delay(2000);
  }
}
