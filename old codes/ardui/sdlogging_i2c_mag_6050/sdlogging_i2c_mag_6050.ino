#define MMC5883MA_XOUT_LOW 0x00
#define MMC5883MA_XOUT_HIGH 0x01
#define MMC5883MA_YOUT_LOW 0x02
#define MMC5883MA_YOUT_HIGH 0x03
#define MMC5883MA_ZOUT_LOW 0x04
#define MMC5883MA_ZOUT_HIGH 0x05
#define MMC5883MA_TEMPERATURE 0x06
#define MMC5883MA_STATUS 0x07
#define MMC5883MA_INTERNAL_CONTROL_0 0x08
#define MMC5883MA_INTERNAL_CONTROL_1 0x09
#define MMC5883MA_INTERNAL_CONTROL_2 0x0A
#define MMC5883MA_X_THRESHOLD 0x0B
#define MMC5883MA_Y_THRESHOLD 0x0C
#define MMC5883MA_Z_THRESHOLD 0x0D
#define MMC5883MA_PRODUCT_ID 0x2F

#include <Wire.h>
#include <SD.h>
#include <SPI.h>

const int mag_I2c = 0x30; //
const int MPU_addr = 0x68;
const int SD_CS = 10; // Pin 10 on Arduino Uno
const int led = 5; //
long values[50], t[9];
double x, y, z, x1, y1, z1, c, xm1, ym1, zm1, gx1, gy1, gz1;
String logFileName;
File logFile;
int i = 0;


void setup() {
  Serial.begin(115200);
  Wire.begin();
  //TWBR = 12;

  if (!SD.begin(SD_CS)) {
    Serial.println("Card failed, or not present");
  }
  pinMode(SD_CS, OUTPUT);
  logFileName = nextLogFile();

  writeRegister(MMC5883MA_INTERNAL_CONTROL_0, 0x08);
  delay(10);
  writeRegister(MMC5883MA_INTERNAL_CONTROL_0, 0x10);
  delay(10);

  writeRegister(MMC5883MA_INTERNAL_CONTROL_0, 0x01); //initiate measurement
  //writeRegister(MMC5883MA_STATUS, 0x0);
  writeRegister(MMC5883MA_INTERNAL_CONTROL_1, 0x00);

  Serial.println("done initialize M");

  writeRegister1(0x6B, 0x00);// set to zero (wakes up the MPU-6050)

  logFile = SD.open(logFileName, FILE_WRITE);
  if (logFile) {
    Serial.println("writing");
    //test0.print("當您取消產品保修時。然後你真正擁有了這個產品。");
    //test0.close(); // close the file
  }
  // if the file didn't open, print an error:
  else {
    Serial.println("error opening file");
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
  writeRegister(MMC5883MA_INTERNAL_CONTROL_0, 0x01);
  mag_data();
  imu6050_data();


  Serial.print(x1, 5);
  Serial.print(", ");
  Serial.print(y1, 5);
  Serial.print(", ");
  Serial.print(z1, 5);
  Serial.print(",   ");
  Serial.print(gx1, 5);
  Serial.print(", ");
  Serial.print(gy1, 5);
  Serial.print(", ");
  Serial.print(gz1, 5);
  Serial.print(",   ");

  Serial.print(xm1, 5);
  Serial.print(", ");
  Serial.print(ym1, 5);
  Serial.print(", ");
  Serial.println(zm1, 5);

  //delay(1);
  if (!logFile) {
    logFile = SD.open(logFileName, FILE_WRITE);
    Serial.println("file opened");
  }
  if (logFile) {
    //Serial.println("writing");
    digitalWrite(led, HIGH);
    logFile.print(x1, 5);
    logFile.print(", ");
    logFile.print(y1, 5);
    logFile.print(", ");
    logFile.print(z1, 5);
    logFile.print(",   ");
    logFile.print(gx1, 5);
    logFile.print(", ");
    logFile.print(gy1, 5);
    logFile.print(", ");
    logFile.print(gz1, 5);
    logFile.print(",   ");
    logFile.print(xm1, 5);
    logFile.print(", ");
    logFile.print(ym1, 5);
    logFile.print(", ");
    logFile.println(zm1, 5);
    //logFile.close(); // close the file
  }
  // if the file didn't open, print an error:
  else {
    Serial.println("error opening test.txt");
    //return;
  }
  //delay(1);
  digitalWrite(led, LOW);
  i++;
  if (i >= 100) {
    logFile.close(); // close the file
    i = 0;
  }
}


void writeRegister(int reg, int data) {
  Wire.beginTransmission(mag_I2c);
  Wire.write(reg);
  Wire.write(data);
  Wire.endTransmission();
}

int readRegister(int reg) {
  Wire.beginTransmission(mag_I2c);
  Wire.write(reg);
  Wire.endTransmission();
  Wire.requestFrom(mag_I2c, 1);
  if (Wire.available() <= 1) {
    return Wire.read();
  }
}
void writeRegister1(int reg, int data) {
  Wire.beginTransmission(MPU_addr);
  Wire.write(reg);
  Wire.write(data);
  Wire.endTransmission();
}
int readRegister1(int reg) {
  Wire.beginTransmission(MPU_addr);
  Wire.write(reg);
  Wire.endTransmission();
  Wire.requestFrom(MPU_addr, 1);
  if (Wire.available() <= 1) {
    return Wire.read();
  }
}

void mag_data() {
  values[20] = readRegister(MMC5883MA_XOUT_LOW);
  values[21] = readRegister(MMC5883MA_XOUT_HIGH);
  values[22] = readRegister(MMC5883MA_YOUT_LOW);
  values[23] = readRegister(MMC5883MA_YOUT_HIGH);
  values[24] = readRegister(MMC5883MA_ZOUT_LOW);
  values[25] = readRegister(MMC5883MA_ZOUT_HIGH);

  xm1 = (values[21] << 8) + values[20];
  ym1 = (values[23] << 8) + values[22];
  zm1 = (values[25] << 8) + values[24];

  xm1 -= 0x8000;
  ym1 -= 0x8000;
  zm1 -= 0x8000;

  xm1 = xm1 / 40.96;
  ym1 = ym1 / 40.96;
  zm1 = zm1 / 40.96;
}

void imu6050_data() {

  values[31] = readRegister1(0x3B);
  values[32] = readRegister1(0x3C);
  values[33] = readRegister1(0x3D);
  values[34] = readRegister1(0x3E);
  values[35] = readRegister1(0x3F);
  values[36] = readRegister1(0x40);
  values[37] = readRegister1(0x43);
  values[38] = readRegister1(0x44);
  values[39] = readRegister1(0x45);
  values[40] = readRegister1(0x46);
  values[41] = readRegister1(0x47);
  values[42] = readRegister1(0x48);

  x1 = values[31] << 8 | values[32]; // 0x3B (ACCEL_XOUT_H) & 0x3C (ACCEL_XOUT_L)
  y1 = values[33] << 8 | values[34];
  z1 = values[35] << 8 | values[36];
  gx1 = values[37] << 8 | values[38];
  gy1 = values[39] << 8 | values[40];
  gz1 = values[41] << 8 | values[42];

  if (x1 >= 0x8000) {
    x1 = x1 -  0x10000;
  }
  if (y1 >= 0x8000) {
    y1 = y1 -  0x10000;
  }
  if (z1 >= 0x8000) {
    z1 = z1 -  0x10000;
  }
  if (gx1 >= 0x8000) {
    gx1 = gx1 -  0x10000;
  }
  if (gy1 >= 0x8000) {
    gy1 = gy1 -  0x10000;
  }
  if (gz1 >= 0x8000) {
    gz1 = gz1 -  0x10000;
  }

  x1 = x1 / 0x4000;
  y1 = y1 / 0x4000;
  z1 = z1 / 0x4000;
  gx1 = gx1 / 131;
  gy1 = gy1 / 131;
  gz1 = gz1 / 131;
}

String nextLogFile(void)
{
  String filename;
  int logn = 0;

  for (int i = 0; i < 999; i++) {
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
