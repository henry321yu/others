#define DEVID_AD                 0x00 //ADXL355BZ
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
#include <Wire.h>

const int SD_CS = 10; // Pin 10 on Arduino Uno
const int acc_I2c = 0x53; //
const int led = 5; //
long values[20], t[9];
double x, y, z, x1, y1, z1, c;
String logFileName;
File logFile;
int i=0;

void setup() {
  Serial.begin(115200);
  Wire.begin();
  //TWBR = 12;

  if (!SD.begin(SD_CS)) {
    Serial.println("Card failed, or not present");
  }
  pinMode(SD_CS, OUTPUT);
  logFileName = nextLogFile(); 

  //adxl355 setting
  writeRegister(POWER_CTL, 0x00);  // writing 0 to to enable sensor
  writeRegister(RANGE, 0xC1);
  writeRegister(SELF_TEST, 0x00);  // writing 3 to to enable
  delay(100);
  logFile = SD.open(logFileName, FILE_WRITE);
  if (logFile) {
    Serial.println("writing");
    //test0.print("當您取消產品保修時。然後你真正擁有了這個產品。");
    //test0.close(); // close the file
  }
  // if the file didn't open, print an error:
  else {
    Serial.println("error opening test.txt");
    //return;
  }
  pinMode(led, OUTPUT);
  pinMode(4, OUTPUT);
  digitalWrite(4, LOW);
  digitalWrite(led, HIGH);
  delay(2000);
  digitalWrite(led, LOW);

  delay(1000);
}
void loop() {

  acc_data();

    
    Serial.print(x, 5);
    Serial.print(", ");
    Serial.print(y, 5);
    Serial.print(", ");
    Serial.println(z, 5);
    
  //delay(1);
  if (!logFile) {
    logFile = SD.open(logFileName, FILE_WRITE);
    Serial.println("file opened");
  }
  if (logFile) {
    //Serial.println("writing");
    digitalWrite(led, HIGH);
    logFile.print(x, 5);
    logFile.print(", ");
    logFile.print(y, 5);
    logFile.print(", ");
    logFile.println(z, 5);
    //logFile.close(); // close the file
  }
  // if the file didn't open, print an error:
  else {
    Serial.println("error opening test.txt");
    //return;
  }
  delay(1);
  digitalWrite(led, LOW);
  i++;
  if (i>=100){
  logFile.close(); // close the file
  i=0;}
}

void writeRegister(int reg, int data) {
  Wire.beginTransmission(acc_I2c);
  Wire.write(reg);
  Wire.write(data);
  Wire.endTransmission();
}

int readRegister(int reg) {
  Wire.beginTransmission(acc_I2c);
  Wire.write(reg);
  Wire.endTransmission();
  Wire.requestFrom(acc_I2c, 1);
  if (Wire.available() <= 1) {
    return Wire.read();
  }
}

void acc_data() {
  values[0] = readRegister(XDATA3);
  values[1] = readRegister(XDATA2);
  values[2] = readRegister(YDATA3);
  values[3] = readRegister(YDATA2);
  values[4] = readRegister(ZDATA3);
  values[5] = readRegister(ZDATA2);

  values[6] = readRegister(XDATA1);
  values[7] = readRegister(YDATA1);
  values[8] = readRegister(ZDATA1);

  //t[0] = readRegister(TEMP2);
  //t[1] = readRegister(TEMP1);

  x = (values[0] << 12) + (values[1] << 4) + (values[6] >> 4);
  y = (values[2] << 12) + (values[3] << 4) + (values[7] >> 4);
  z = (values[4] << 12) + (values[5] << 4) + (values[8] >> 4);

  if (x >= 0x80000) {
    x = x - (2 * 0x80000);
  }
  if (y >= 0x80000) {
    y = y - (2 * 0x80000);
  }
  if (z >= 0x80000) {
    z = z - (2 * 0x80000);
  }

  //c = (t[0] << 8) + t[1];
  //c = ((1852 - c) / 9.05) + 25.7;

  x = x * 0.00000390625;
  y = y * 0.00000390625;
  z = z * 0.00000390625;
}

String nextLogFile(void)
{
  String filename;
  int logn = 0;
  
  for (int i = 0; i < 999; i++){
    // Construct a file with PREFIX[Index].SUFFIX
    filename = String("logg");
    filename += String(logn);
    filename += ".";
    filename += String("txt");
    // If the file name doesn't exist, return it
    if (!SD.exists(filename))
    {
      return filename;
    }
    // Otherwise increment the index, and try again
    logn++;
  }

  return "";
}
