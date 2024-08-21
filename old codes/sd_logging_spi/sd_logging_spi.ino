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

#include <SD.h>
#include <SPI.h>

const int SD_CS = 8; // Pin 10 on Arduino Uno
const int acc_CS = 9; //
long values[20],t[9];
double x, y, z, c;
File test0;

void setup() {  
  Serial.begin(9600);
  SPI.begin(); 

  if (!SD.begin(SD_CS)) {
    Serial.println("Card failed, or not present");
  }
  pinMode(SD_CS, OUTPUT);
  digitalWrite(SD_CS, HIGH);
    
  SPI.setClockDivider(SPI_CLOCK_DIV16);
  SPI.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE0)); // 1000khz clock

  pinMode(acc_CS, OUTPUT);
  digitalWrite(acc_CS, HIGH);

  writeRegister(POWER_CTL, 0x00);  // writing 0 to to enable sensor
  writeRegister(RANGE, 0xC1);
  writeRegister(SELF_TEST, 0x00);  // writing 3 to to enable

  digitalWrite(SD_CS, LOW);
  test0 = SD.open("test0.txt", FILE_WRITE);
  if (test0) {    
    Serial.println("writing");
    test0.print("當您取消產品保修時，如果出現問題，您將放棄起訴製造商的權利，並對接下來發生的任何事情承擔全部責任。然後你真正擁有了這個產品。");
    //test0.close(); // close the file
  }
  // if the file didn't open, print an error:
  else {
    Serial.println("error opening test.txt");
  }
  digitalWrite(SD_CS, HIGH);

  
  delay(500);
}
void loop() {
  
  acc_data();
  
  Serial.print(x,5);
  Serial.print(',');
  Serial.print(y,5);
  Serial.print(',');
  Serial.print(z,5);
  Serial.print('\n');

  delay(500);

  digitalWrite(SD_CS, LOW);

  if (!test0) {
    test0 = SD.open("test0.txt", FILE_WRITE);
    Serial.println("loop opened");
    }
    
  //File test0 = SD.open("test0.txt", FILE_WRITE);
  if (test0) {    
    Serial.println("writing");
    test0.print(x);
    test0.print(",");
    test0.print(y);
    test0.print(",");
    test0.println(z);
    test0.close(); // close the file
  }
  // if the file didn't open, print an error:
  else {
    Serial.println("error opening test.txt");
  }
  digitalWrite(SD_CS, HIGH);
    delay(10);
}

byte readRegister (byte thisRegister) {
  byte inByte = 0 ;
  digitalWrite(acc_CS, LOW);
  SPI.transfer((thisRegister << 1) | 1);
  inByte = SPI.transfer(0x00);
  digitalWrite(acc_CS, HIGH);
  return inByte;
}
void writeRegister (byte thisRegister, byte value) {
  digitalWrite(acc_CS, LOW);
  SPI.transfer(thisRegister << 1);
  SPI.transfer(value);
  digitalWrite(acc_CS, HIGH);
}
void acc_data(){
  values[0] = readRegister(XDATA3);
  values[1] = readRegister(XDATA2);
  values[2] = readRegister(YDATA3);
  values[3] = readRegister(YDATA2);
  values[4] = readRegister(ZDATA3);
  values[5] = readRegister(ZDATA2);

  values[6] = readRegister(XDATA1);
  values[7] = readRegister(YDATA1);
  values[8] = readRegister(ZDATA1);

  t[0] = readRegister(TEMP2);
  t[1] = readRegister(TEMP1);

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
  
  x=x*0.0000039;
  y=y*0.0000039;
  z=z*0.0000039;
  }
