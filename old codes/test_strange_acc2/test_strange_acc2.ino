#define TCAADDR 0x70

#include <Wire.h>
#include <SPI.h>
#include"register.h"
// small board change:sd,led,beeper,hall,mpu6050 set&read tcaselect(2)
const int mag_I2c = 0x30; //
const int MPU_addr = 0x68;
const int acc_I2c = 0x1D; //0x53
const int SD_CS = 10; // teensy builtin
const int led = 17; //
const int beeper = 4;
int ID, mn[6] = {2, 3, 4, 5, 6, 7}, delayy = 0, setF = 100;
long values[12], t[12];
double x[3], y[3], z[3], xm[7], ym[7], zm[7], gx1, gy1, gz1, t0, time = 0, f, tm[7], ta[3], tg; //, x1, y1, z1
String logFileName, accdata, gyrdata, magdata, logdata, mag2data, tmdata, halldata;// Rotordata;
File logFile;
unsigned long i = 0, t1, beepert;
const int H1 = A8, H2 = A9; //hall need wire
int lastR, Read, lastR2, Read2, maxsize = 262144000;//250mb = 250*2^20;
double Rotor = 0, Rotor2 = 0;

void setup() {
  Serial.begin(115200);
  Wire.begin();
  Wire.setClock(400000);


  //adxl355 setting
  tcaselect(1);
  writeRegister(acc_I2c, RESET, 0x52);  // reset sensor
  delay(100);
  writeRegister(acc_I2c, POWER_CTL, 0x00);  // writing 0 to to enable sensor
  delay(30);
  writeRegister(acc_I2c, RANGE, 0x01);
  delay(30);
  writeRegister(acc_I2c, SELF_TEST, 0x00);  // writing 3 to to enable self test
  delay(30);

  //adxl355 2 setting
  tcaselect(0);
  writeRegister(acc_I2c, RESET, 0x52);  // reset sensor
  delay(100);
  writeRegister(acc_I2c, POWER_CTL, 0x00);  // writing 0 to to enable sensor
  delay(30);
  writeRegister(acc_I2c, RANGE, 0x01);
  delay(30);
  writeRegister(acc_I2c, SELF_TEST, 0x00);  // writing 3 to to enable self test
  delay(30);

  Serial.println(F("done initialize"));
  delay(2000);
}
void loop() {
  accdata = String(x[1], 5) + ", " + String(y[1], 5) + ", " + String(z[1], 5) + ", " + String(x[2], 5) + ", " + String(y[2], 5) + ", " + String(z[2], 5);
  Serial.println(accdata);
  delay(2000);

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

void readmultiRegister(int fst, int num) {
  int k = 0;
  memset(values, 0, sizeof(values));
  Wire.beginTransmission(ID);
  Wire.write(fst);
  Wire.endTransmission();
  Wire.requestFrom(ID, num);
  while (Wire.available() && k < num)
  {
    values[k++] = Wire.read();
  }
}


void acc_data() {
  ID = acc_I2c;

  for (int j = 0; j < 2; j++) {
    tcaselect(j);
    
    readmultiRegister(TEMP2, 11);

    ta[0] = values[0] << 8 | values[1];
    x[0] = values[2] << 12 | values[3] << 4 | values[4] >> 4;
    y[0] = values[5] << 12 | values[6] << 4 | values[7] >> 4;
    z[0] = values[8] << 12 | values[9] << 4 | values[10] >> 4;

    if (x[0] >= 0x80000)
      x[0] = x[0] - (2 * 0x80000);
    if (y[0] >= 0x80000)
      y[0] = y[0] - (2 * 0x80000);
    if (z[0] >= 0x80000)
      z[0] = z[0] - (2 * 0x80000);

    ta[0] = ((1852 - ta[0]) / 9.05);

    x[0] = x[0] / 256000;  //2G
    y[0] = y[0] / 256000;
    z[0] = z[0] / 256000;
    //x[0] = x[0] / 128000;  //4G
    //y[0] = y[0] / 128000;
    //z[0] = z[0] / 128000;

    if (j == 0) { //switching data to acc1,acc2
      x[2] = x[0];
      y[2] = y[0];
      z[2] = z[0];
      ta[2] = ta[0] + 21;
    }
    else {
      x[1] = x[0];
      y[1] = y[0];
      z[1] = z[0];
      ta[1] = ta[0]  + 21;
    }
  }
}

void tcaselect(uint8_t i) {
  if (i > 7) return;
  Wire.beginTransmission(TCAADDR);
  Wire.write(1 << i);
  Wire.endTransmission();
}
