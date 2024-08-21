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

#define ACCEL_XOUT_H  0x3B //MPU9250
#define ACCEL_XOUT_L  0x3C
#define ACCEL_YOUT_H  0x3D
#define ACCEL_YOUT_L  0x3E
#define ACCEL_ZOUT_H  0x3F
#define ACCEL_ZOUT_L  0x40
#define WHO_AM_I      0x75
#define PWR_MGMT_1    0x6B
#define PWR_MGMT_2    0x6C
#define ST_X_ACCEL    0x0D
#define ST_Y_ACCEL    0x0E
#define ST_Z_ACCEL    0x0F
#define ACCEL_CONFIG  0x1C
#define ACCEL_CONFIG2 0x1D

#include <SD.h>
#include <SPI.h>
#include <WSWire.h>

const int SD_CS = 10; // Pin 10 on Arduino Uno
const int acc_I2c = 0x1D; //
const int IMU_I2c = 0x68; //
const int led = 4; //
long values[20], t[9];
double x, y, z, x1, y1, z1, c;
File test0;

void setup() {
  Serial.begin(115200);
  Wire.begin();
  //TWBR = 12;

  if (!SD.begin(SD_CS)) {
    Serial.println("Card failed, or not present");
  }
  pinMode(SD_CS, OUTPUT);

  //adxl355 setting
  writeRegister(POWER_CTL, 0x00);  // writing 0 to to enable sensor
  writeRegister(RANGE, 0xC1);
  writeRegister(SELF_TEST, 0x00);  // writing 3 to to enable
  delay(200);
  //imu setting
  //writeRegister1(PWR_MGMT_1, 0x80); //reset
  writeRegister1(PWR_MGMT_2, 0x00);
  writeRegister1(PWR_MGMT_1, 0x01); //enable all axis
  //writeRegister1(ACCEL_CONFIG, 0xE0); // Accel self-test
  //writeRegister(ASTC, 0x01);

  delay(200);
  //digitalWrite(SD_CS, LOW);
  test0 = SD.open("test0.txt", FILE_WRITE);
  if (test0) {
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
  digitalWrite(led, LOW);



  delay(200);
}
void loop() {

  acc_data();

  Serial.print(x, 5);
  Serial.print(", ");
  Serial.print(y, 5);
  Serial.print(", ");
  Serial.print(z, 5);
  Serial.print(",     ");
  delay(10);
  imu_data();

  Serial.print(x1, 5);
  Serial.print(", ");
  Serial.print(y1, 5);
  Serial.print(", ");
  Serial.print(z1, 5);
  Serial.print('\n');  
  delay(5);

  if (!test0) {
    test0 = SD.open("test0.txt", FILE_WRITE);
    Serial.println("file opened");
  }
  if (test0) {
    Serial.println("writing");
    digitalWrite(led, HIGH);
    test0.print(x, 5);
    test0.print(", ");
    test0.print(y, 5);
    test0.print(", ");
    test0.print(z, 5);
    test0.print(",     ");
    test0.print(x1, 5);
    test0.print(", ");
    test0.print(y1, 5);
    test0.print(", ");
    test0.println(z1, 5);
    test0.close(); // close the file
  }
  // if the file didn't open, print an error:
  else {
    Serial.println("error opening test.txt");
  }
  delay(500);
  digitalWrite(led, LOW);
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
  Wire.endTransmission();
}

void writeRegister1(int reg1, int data1) {
  Wire.beginTransmission(IMU_I2c);
  Wire.write(reg1);
  Wire.write(data1);
  Wire.endTransmission();
}

int readRegister1(int reg1) {
  Wire.beginTransmission(IMU_I2c);
  Wire.write(reg1);
  Wire.endTransmission();
  Wire.requestFrom(IMU_I2c, 1);
  if (Wire.available() <= 1) {
    return Wire.read();
  }
  Wire.endTransmission();
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

  t[0] = readRegister(TEMP2);
  t[1] = readRegister(TEMP1);

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

  c = (t[0] << 8) + t[1];
  c = ((1852 - c) / 9.05) + 25.7;

  x = x * 0.0000039;
  y = y * 0.0000039;
  z = z * 0.0000039;
}

void imu_data() {
  values[10] = readRegister1(ACCEL_XOUT_H);
  values[11] = readRegister1(ACCEL_XOUT_L);
  values[12] = readRegister1(ACCEL_YOUT_H);
  values[13] = readRegister1(ACCEL_YOUT_L);
  values[14] = readRegister1(ACCEL_ZOUT_H);
  values[15] = readRegister1(ACCEL_ZOUT_L);

  x1 = (values[10] << 8) + values[11];
  y1 = (values[12] << 8) + values[13];
  z1 = (values[14] << 8) + values[15];

  if (x1 >= 0x8000) {
    x1 = x1 -  0x10000;
  }
  if (y1 >= 0x8000) {
    y1 = y1 -  0x10000;
  }
  if (z1 >= 0x8000) {
    z1 = z1 -  0x10000;
  }

  x1 = x1/0x4000;
  y1 = y1/0x4000;
  z1 = z1/0x4000;
}
