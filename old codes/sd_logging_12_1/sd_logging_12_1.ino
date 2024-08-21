#define MMC5883MA_XOUT_LOW 0x00 //MMC5883MA-B
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

#define TCAADDR 0x70

#include <Wire.h>
#include <SD.h>
#include <SPI.h>

const int mag_I2c = 0x30; //
const int MPU_addr = 0x68;
const int acc_I2c = 0x1D; //0x53
const int SD_CS = 10; // Pin 10 on Arduino Uno
const int led = 5; //
int ID;
long values[60], t[10];
double x, y, z, c, xm1, ym1, zm1, gx1, gy1, gz1, xm2, ym2, zm2;//, x1, y1, z1
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

  //MMC5883MA 1 setting
  tcaselect(0);
  writeRegister(mag_I2c, MMC5883MA_INTERNAL_CONTROL_0, 0x08);
  delay(10);
  writeRegister(mag_I2c, MMC5883MA_INTERNAL_CONTROL_0, 0x10);
  delay(10);

  writeRegister(mag_I2c , MMC5883MA_INTERNAL_CONTROL_0, 0x01); //initiate measurement
  //writeRegister(mag_I2c ,MMC5883MA_STATUS, 0x0);
  writeRegister(mag_I2c , MMC5883MA_INTERNAL_CONTROL_1, 0x00);

  //MMC5883MA 2 setting
  tcaselect(3);
  writeRegister(mag_I2c, MMC5883MA_INTERNAL_CONTROL_0, 0x08);
  delay(10);
  writeRegister(mag_I2c, MMC5883MA_INTERNAL_CONTROL_0, 0x10);
  delay(10);

  writeRegister(mag_I2c , MMC5883MA_INTERNAL_CONTROL_0, 0x01); //initiate measurement
  //writeRegister(mag_I2c ,MMC5883MA_STATUS, 0x0);
  writeRegister(mag_I2c , MMC5883MA_INTERNAL_CONTROL_1, 0x00);

  
  //wakes up the MPU-6050
  tcaselect(1);
  writeRegister(MPU_addr, 0x6B, 0x00);// set to zero (wakes up the MPU-6050)

  //adxl355 setting
  tcaselect(2);
  writeRegister(acc_I2c, POWER_CTL, 0x00);  // writing 0 to to enable sensor
  writeRegister(acc_I2c, RANGE, 0xC1);
  writeRegister(acc_I2c, SELF_TEST, 0x00);  // writing 3 to to enable

  Serial.println("done initialize");


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

  //delay(1);
}
void loop() {
  mag_data();
  imu6050_data();
  acc_data();
  mag_data2();


  Serial.print(x, 5);
  Serial.print(", ");
  Serial.print(y, 5);
  Serial.print(", ");
  Serial.print(z, 5);
  Serial.print(",   ");

  /*Serial.print(x1, 5);
  Serial.print(", ");
  Serial.print(y1, 5);
  Serial.print(", ");
  Serial.print(z1, 5);
  Serial.print(",   ");*/
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
  Serial.print(zm1, 5);
  Serial.print(",   ");

  Serial.print(xm2, 5);
  Serial.print(", ");
  Serial.print(ym2, 5);
  Serial.print(", ");
  Serial.println(zm2, 5);

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
    logFile.print(z, 5);
    logFile.print(",   ");

    /*logFile.print(x1, 5);
    logFile.print(", ");
    logFile.print(y1, 5);
    logFile.print(", ");
    logFile.print(z1, 5);
    logFile.print(",   ");*/
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
    logFile.print(zm1, 5);
    logFile.print(",   ");

    logFile.print(xm2, 5);
    logFile.print(", ");
    logFile.print(ym2, 5);
    logFile.print(", ");
    logFile.println(zm2, 5);
    //logFile.close(); // close the file
  }
  // if the file didn't open, print an error:
  else {
    Serial.println("error opening test.txt");
    //return;
  }
  delay(10);
  digitalWrite(led, LOW);
  i++;
  if (i >= 100) {
    logFile.close(); // close the file
    i = 0;
  }
}


void writeRegister(int ID, int reg, int data) {
  Wire.beginTransmission(ID);
  Wire.write(reg);
  Wire.write(data);
  Wire.endTransmission();
}

int readRegister(int reg) {
  Wire.beginTransmission(ID);
  Wire.write(reg);
  Wire.endTransmission();
  Wire.requestFrom(ID, 1);
  if (Wire.available() <= 1) {
    return Wire.read();
  }
}

void mag_data() {
  tcaselect(0);
  writeRegister(mag_I2c, MMC5883MA_INTERNAL_CONTROL_0, 0x01);
  ID = mag_I2c;
  values[20] = readRegister(MMC5883MA_XOUT_LOW);
  values[21] = readRegister(MMC5883MA_XOUT_HIGH);
  values[22] = readRegister(MMC5883MA_YOUT_LOW);
  values[23] = readRegister(MMC5883MA_YOUT_HIGH);
  values[24] = readRegister(MMC5883MA_ZOUT_LOW);
  values[25] = readRegister(MMC5883MA_ZOUT_HIGH);

  xm1 = values[21] << 8 | values[20];
  ym1 = values[23] << 8 | values[22];
  zm1 = values[25] << 8 | values[24];

  xm1 -= 0x8000;
  ym1 -= 0x8000;
  zm1 -= 0x8000;

  xm1 = xm1 / 40.96;
  ym1 = ym1 / 40.96;
  zm1 = zm1 / 40.96;
}
void mag_data2() {
  tcaselect(3);
  writeRegister(mag_I2c, MMC5883MA_INTERNAL_CONTROL_0, 0x01);
  ID = mag_I2c;
  values[50] = readRegister(MMC5883MA_XOUT_LOW);
  values[51] = readRegister(MMC5883MA_XOUT_HIGH);
  values[52] = readRegister(MMC5883MA_YOUT_LOW);
  values[53] = readRegister(MMC5883MA_YOUT_HIGH);
  values[54] = readRegister(MMC5883MA_ZOUT_LOW);
  values[55] = readRegister(MMC5883MA_ZOUT_HIGH);

  xm2 = values[51] << 8 | values[50];
  ym2 = values[53] << 8 | values[52];
  zm2 = values[55] << 8 | values[54];

  xm2 -= 0x8000;
  ym2 -= 0x8000;
  zm2 -= 0x8000;

  xm2 = xm2 / 40.96;
  ym2 = ym2 / 40.96;
  zm2 = zm2 / 40.96;
}

void imu6050_data() {
  tcaselect(1);
  ID = MPU_addr;
  /*values[31] = readRegister(0x3B);
  values[32] = readRegister(0x3C);
  values[33] = readRegister(0x3D);
  values[34] = readRegister(0x3E);
  values[35] = readRegister(0x3F);
  values[36] = readRegister(0x40);*/
  values[37] = readRegister(0x43);
  values[38] = readRegister(0x44);
  values[39] = readRegister(0x45);
  values[40] = readRegister(0x46);
  values[41] = readRegister(0x47);
  values[42] = readRegister(0x48);

  /*x1 = values[31] << 8 | values[32]; // 0x3B (ACCEL_XOUT_H) & 0x3C (ACCEL_XOUT_L)
  y1 = values[33] << 8 | values[34];
  z1 = values[35] << 8 | values[36];*/
  gx1 = values[37] << 8 | values[38];
  gy1 = values[39] << 8 | values[40];
  gz1 = values[41] << 8 | values[42];

  /*if (x1 >= 0x8000) {
    x1 = x1 -  0x10000;
  }
  if (y1 >= 0x8000) {
    y1 = y1 -  0x10000;
  }
  if (z1 >= 0x8000) {
    z1 = z1 -  0x10000;
  }*/
  if (gx1 >= 0x8000) {
    gx1 = gx1 -  0x10000;
  }
  if (gy1 >= 0x8000) {
    gy1 = gy1 -  0x10000;
  }
  if (gz1 >= 0x8000) {
    gz1 = gz1 -  0x10000;
  }

  /*x1 = x1 / 0x4000;
  y1 = y1 / 0x4000;
  z1 = z1 / 0x4000;*/
  gx1 = gx1 / 131;
  gy1 = gy1 / 131;
  gz1 = gz1 / 131;

  /*for (int k = 0; k > 5; k++) {
    if (t[k] >= 0x8000) {
      Serial.print(k);
      t[k] -= 0x10000;
    }
    }
    x1 = t[0] / 0x4000;
    y1 = t[1] / 0x4000;
    z1 = t[2] / 0x4000;
    gx1 = t[3] / 131;
    gy1 = t[4] / 131;
    gz1 = t[5] / 131;*/
}

void acc_data() {
  tcaselect(2);
  ID = acc_I2c;
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

  x = values[0] << 12 | values[1] << 4 | values[6] >> 4;
  y = values[2] << 12 | values[3] << 4 | values[7] >> 4;
  z = values[4] << 12 | values[5] << 4 | values[8] >> 4;

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

void tcaselect(uint8_t i) {
  if (i > 7) return;
 
  Wire.beginTransmission(TCAADDR);
  Wire.write(1 << i);
  Wire.endTransmission();  
}
