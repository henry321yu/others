#include <SPI.h>
#include <ADIS16470.h>

const int acc16470_CS = 10; //
double x, y, z, c;
int16_t regData[32];
byte accdataN = 24;
int32_t gxo, gyo, gzo, axo, ayo, azo = 0;
int32_t axo2, ayo2, azo2 = 0;
double ax, ay, az;
byte regread0 , regread1, regread2, regread3;

ADIS16470 IMU(10, 2, 6); // Chip Select, Data Ready, Reset Pin Assignments

void setup() {
  Serial.begin(115200); // Initialize serial output via USB
  IMU.configSPI(); // Configure SPI communication
  delay(500); // Give the part time to start up
  IMU.regWrite(MSC_CTRL, 0xC1);  // Enable Data Ready, set polarity
  //IMU.regWrite(FILT_CTRL, 0x04); // Set digital filter
  //IMU.regWrite(DEC_RATE, 0x03); // Disable decimation
  IMU.regWrite(FILT_CTRL, 0x00); // Set digital filter
  IMU.regWrite(DEC_RATE, 0x00); // Disable decimation


  //SPI.setClockDivider(SPI_CLOCK_DIV16);
  //SPI.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE0)); // 1000khz clock
  //pinMode(acc16470_CS, OUTPUT);
  //digitalWrite(acc16470_CS, HIGH);

  delay(500);
}


void loop() {

  //accregRead();

  /*regData[0] = readRegister(Z_ACCL_LOW);
    regData[1] = readRegister(Z_ACCL_LOW + 1);
    regData[2] = readRegister(Z_ACCL_OUT);
    regData[3] = readRegister(Z_ACCL_OUT + 1);*/

  /*regData[0] = IMU.regRead(Z_ACCL_LOW);
    regData[1] = IMU.regRead(Z_ACCL_LOW + 1);
    regData[2] = IMU.regRead(Z_ACCL_OUT);
    regData[3] = IMU.regRead(Z_ACCL_OUT + 1);*/

  regread0 = 0x18;//LOW1
  regread1 = regread0 + 1; //LOW2
  regread2 = regread1 + 1; //OUT1
  regread3 = regread2 + 1; //OUT2

  regData[0] = regRead8(regread0);
  regData[1] = regRead8(regread1);
  //regData[2] = regRead8(regread2);
  //regData[3] = regRead8(regread3);

  /*regData[0] = IMU.regRead(regread0);
    regData[1] = IMU.regRead(regread1);
    regData[2] = IMU.regRead(regread2);
    regData[3] = IMU.regRead(regread3);*/


  /*if (regData[2] > 0x7F) {
    regData[2] = regData[2] - 0xFF;
    }*/

  //azo = (regData[2] << 8) | (regData[3] & 0xFF);
  //azo |= regData[3] << 4;
  //azo |= regData[0] >> 4;
  //azo |= regData[1] >> 12;

  //ayo = (regData[0] << 8 ) | regData[1];
  ayo = IMU.regRead(regread0);
  azo = IMU.regRead(regread2) << 16;
  axo = azo + ayo;

  az = axo;
  az = az / 262144000;

  Serial.print(az);
  Serial.print("\t");
  Serial.print(azo);
  Serial.print("\t");
  Serial.print(axo);
  Serial.print("\t");
  //Serial.print(ayo);
  Serial.print("\t");
  //Serial.print(azo, HEX);
  Serial.print("\t");
  //Serial.print(ayo, HEX);
  Serial.print("\t");
  //Serial.print(azo, BIN);
  Serial.print("\t");
  //Serial.print(axo, BIN);
  Serial.print("\t");
  //Serial.print(ayo, BIN);
  Serial.print("\t");

  /*Serial.print(regData[0], BIN);
    Serial.print("\t");
    Serial.print(regData[1], BIN);
    Serial.print("\t");
    Serial.print(regData[2], BIN);
    Serial.print("\t");
    Serial.print(regData[3], BIN);
    Serial.print("\t");

    Serial.print(regData[0]);
    Serial.print("\t");
    Serial.print(regData[1]);
    Serial.print("\t");
    Serial.print(regData[2]);
    Serial.print("\t");
    Serial.print(regData[3]);
    Serial.print("\t");*/



  Serial.print("\n");
  //delayMicroseconds(2500); // Delay to not violate read rate
  //delayMicroseconds(2500); // Delay to not violate read rate
  delay(20); // Delay to not violate read rate
  //while (1);
}



int8_t regRead8(uint8_t regAddr) {
  int _stall = 20;
  //Read registers using SPI

  // Write register address to be read
  digitalWrite(acc16470_CS, LOW); // Set CS low to enable device
  SPI.transfer(regAddr); // Write address over SPI bus
  uint8_t _msbData = SPI.transfer(0x00); // Send (0x00) and place upper byte into variable
  digitalWrite(acc16470_CS, HIGH); // Set CS high to disable device
  delayMicroseconds(_stall); // Delay to not violate read rate
  int8_t _dataOut = _msbData;
  return (_dataOut);
}



void accregRead() {
  //Read registers using SPI

  // Write register address to be read
  digitalWrite(acc16470_CS, LOW); // Set CS low to enable device
  SPI.transfer(Y_ACCL_LOW); // Write address over SPI bus
  //SPI.transfer(0x68); // Write address over SPI bus
  //SPI.transfer(0x00,2); // Write 0x00 to the SPI bus fill the 16 bit transaction requirement
  //SPI.transfer(0x00); // Write 0x00 to the SPI bus fill the 16 bit transaction requirement
  //SPI.transfer(0x00); // Write 0x00 to the SPI bus fill the 16 bit transaction requirement
  //SPI.transfer(0x00); // Write 0x00 to the SPI bus fill the 16 bit transaction requirement
  //digitalWrite(acc16470_CS, HIGH); // Set CS high to disable device

  //  delayMicroseconds(500); // Delay to not violate read rate

  // Read data from requested register
  //  digitalWrite(acc16470_CS, LOW); // Set CS low to enable device
  //for (byte i = 0; i < (accdataN - 1); i++) {
  regData[0] = SPI.transfer16(0x00); // Send (0x00) and place upper byte into variable
  regData[1] = SPI.transfer16(0x00);
  regData[2] = SPI.transfer16(0x00);
  regData[3] = SPI.transfer16(0x00);
  //}
  digitalWrite(acc16470_CS, HIGH); // Set CS high to disable device
  delayMicroseconds(500); // Delay to not violate read rate
  SPI.endTransaction();
}

////////////////////

byte readRegister (byte thisRegister) {
  byte inByte = 0 ;
  digitalWrite(acc16470_CS, LOW);
  SPI.transfer(thisRegister);
  inByte = SPI.transfer(0x00);
  digitalWrite(acc16470_CS, HIGH);
  return inByte;
}


int16_t regRead(uint8_t regAddr) {
  int _stall = 20;
  //Read registers using SPI

  // Write register address to be read
  digitalWrite(acc16470_CS, LOW); // Set CS low to enable device
  SPI.transfer(regAddr); // Write address over SPI bus
  SPI.transfer(0x00); // Write 0x00 to the SPI bus fill the 16 bit transaction requirement
  digitalWrite(acc16470_CS, HIGH); // Set CS high to disable device

  delayMicroseconds(_stall); // Delay to not violate read rate

  // Read data from requested register
  digitalWrite(acc16470_CS, LOW); // Set CS low to enable device
  uint8_t _msbData = SPI.transfer(0x00); // Send (0x00) and place upper byte into variable
  uint8_t _lsbData = SPI.transfer(0x00); // Send (0x00) and place lower byte into variable
  digitalWrite(acc16470_CS, HIGH); // Set CS high to disable device
  delayMicroseconds(_stall); // Delay to not violate read rate
  int16_t _dataOut = (_msbData << 8) | (_lsbData & 0xFF); // Concatenate upper and lower bytes
  //int8_t _dataOut = _msbData;
  return (_dataOut);
}


////////////////////
