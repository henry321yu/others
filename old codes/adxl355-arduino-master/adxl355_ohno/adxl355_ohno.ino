#include <SPI.h>

#define DEVID_AD                 0x00
#define DEVID_MST                0x01
#define PARTID                   0x02
#define REVID                    0x03
#define STATUS                   0x04
#define FIFO_ENTRIES             0x05
#define TEMP2                    0x06
#define TEMP1                    0x07
#define XDATA3                   0x08
#define XDATA2                   0x09
#define XDATA1                   0x0A
#define YDATA3                   0x0B
#define YDATA2                   0x0C
#define YDATA1                   0x0D
#define ZDATA3                   0x0E
#define ZDATA2                   0x0F
#define ZDATA1                   0x10
#define FIFO_DATA                0x11
#define OFFSET_X_H               0x1E
#define OFFSET_X_L               0x1F
#define OFFSET_Y_H               0x20
#define OFFSET_Y_L               0x21
#define OFFSET_Z_H               0x22
#define OFFSET_Z_L               0x23
#define ACT_EN                   0x24
#define ACT_THRESH_H             0x25
#define ACT_THRESH_L             0x26
#define ACT_COUNT                0x27
#define FILTER                   0x28
#define FIFO_SAMPLES             0x29
#define INT_MAP                  0x2A
#define SYNC                     0x2B
#define RANGE                    0x2C
#define POWER_CTL                0x2D
#define SELF_TEST                0x2E
#define RESET                    0x2F

const int ID = 0x00;
const int chipSelectPin = 9;
long values[20],t[9];
double x, y, z,x0,y0,z0,c;

void setup() {
  Serial.begin(115200);

  SPI.begin();
  SPI.setClockDivider(SPI_CLOCK_DIV16);
  SPI.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE0)); // 1000khz clock

  pinMode(chipSelectPin, OUTPUT);
  digitalWrite(chipSelectPin, HIGH);

  // writing 0 to to enable sensor
  writeRegister(POWER_CTL, 0x00);
  writeRegister(RANGE, 0xC1);
  writeRegister(SELF_TEST, 0x00);
  //writeRegister(FILTER, 
  //writeRegister(RESET, 0x52);
  

  delay(100);
}

void loop() {
  /*int temp = (readRegister(TEMP)); // << 8) | (readRegister(TEMP1))) ;
    temp = (temp<<8)|(readRegister(TEMP1));
    Serial.println(((1852 - temp)/9.05) + 19.21);
    Serial.println(readRegister(ID));


    digitalWrite(chipSelectPin, LOW);
    SPI.transfer(XDATA3);
    values[0] = SPI.transfer(0x00);
    digitalWrite(chipSelectPin, HIGH);
    //
    digitalWrite(chipSelectPin, LOW);
    SPI.transfer(XDATA2);
    values[1] = SPI.transfer(0x00);
    digitalWrite(chipSelectPin, HIGH);
    //
    digitalWrite(chipSelectPin, LOW);
    SPI.transfer(YDATA3);
    values[2] = SPI.transfer(0x00);
    digitalWrite(chipSelectPin, HIGH);
    //
    digitalWrite(chipSelectPin, LOW);
    SPI.transfer(YDATA2);
    values[3] = SPI.transfer(0x00);
    digitalWrite(chipSelectPin, HIGH);
    //
    digitalWrite(chipSelectPin, LOW);
    SPI.transfer(ZDATA3);
    values[4] = SPI.transfer(0x00);
    digitalWrite(chipSelectPin, HIGH);
    //
    digitalWrite(chipSelectPin, LOW);
    SPI.transfer(ZDATA2);
    values[5] = SPI.transfer(0x00);
    digitalWrite(chipSelectPin, HIGH)*/


  values[0] = readRegister(XDATA3);
  values[1] = readRegister(XDATA2);
  values[2] = readRegister(YDATA3);
  values[3] = readRegister(YDATA2);
  values[4] = readRegister(ZDATA3);
  values[5] = readRegister(ZDATA2);

  values[6] = readRegister(XDATA1);
  values[7] = readRegister(YDATA1);
  values[8] = readRegister(ZDATA1);
  
  values[9] = readRegister(OFFSET_X_H);
  values[10] = readRegister(OFFSET_X_L);
  values[11] = readRegister(OFFSET_Y_H);
  values[12] = readRegister(OFFSET_Y_L);
  values[13] = readRegister(OFFSET_Z_H);
  values[14] = readRegister(OFFSET_Z_L);

  t[0] = readRegister(TEMP2);
  t[1] = readRegister(TEMP1);
  t[2] = readRegister(XDATA1);
  t[3] = readRegister(ZDATA3);
  t[4] = readRegister(ZDATA2);
  t[5] = readRegister(ZDATA1);
  
  x0= (values[9] << 8) + values[10];
  y0= (values[11] << 8) + values[12];
  z0= (values[13] << 8) + values[14];

  x = (values[0] << 12) + (values[1]<<4) + (values[6]>>4);
  y = (values[2] << 12) + (values[3]<<4) + (values[7]>>4);
  z = (values[4] << 12) + (values[5]<<4) + (values[8]>>4);

  if (x >= 0x80000) {
    x = x-(2*0x80000);
  }
  if (y >= 0x80000) {
    y = y-(2*0x80000);
  }
  if (z >= 0x80000) {
    z = z-(2*0x80000);
  }
  
  c=(t[0]<<8)+t[1];
  c=((1852 - c)/9.05)+25.7;
  
  x+=x0;
  y+=y0;
  z+=z0;
  
  x=x*0.0000039;
  y=y*0.0000039;
  z=z*0.0000039;
  
  Serial.print(x,5);
  Serial.print(',');
  Serial.print(y,5);
  Serial.print(',');
  Serial.print(z,5);
  /*Serial.print('\t');
  Serial.print(x0);
  Serial.print(',');
  Serial.print(y0);
  Serial.print(',');
  Serial.print(z0);  
  Serial.print('\t');
  Serial.print(c);
  Serial.print('\t');
  Serial.print(t[0]);
  Serial.print('\t');
  Serial.print(t[1]);
  Serial.print('\t');
  Serial.print(t[2]);
  Serial.print('\t');
  Serial.print(t[3]);
  Serial.print('\t');
  Serial.print(t[4]);
  Serial.print('\t');
  Serial.print(t[5]);*/
  Serial.print('\n');
  delay(7);
}

byte readRegister (byte thisRegister) {
  byte inByte = 0 ;
  digitalWrite(chipSelectPin, LOW);
  SPI.transfer((thisRegister << 1) | 1);
  inByte = SPI.transfer(0x00);
  digitalWrite(chipSelectPin, HIGH);
  return inByte;
}
void writeRegister (byte thisRegister, byte value) {
  digitalWrite(chipSelectPin, LOW);
  SPI.transfer(thisRegister << 1);
  SPI.transfer(value);
  digitalWrite(chipSelectPin, HIGH);
}
