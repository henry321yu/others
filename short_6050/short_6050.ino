#include <Wire.h>

const int MPU_addr = 0x68;
long values[50];
String logdata, accdata, gyrdata;
double x[3], y[3], z[3], gx[3], gy[3], gz[3], ta[3];
int ID;

void setup() {
  Serial.begin(115200); // Initialize serial output via USB
  Wire.begin();
  
  //wakes up the MPU-6050
  writeRegister(MPU_addr, 0x6B, 0x80);// reset
  delay(100);
  writeRegister(MPU_addr, 0x6B, 0x00);// set to zero (wakes up the MPU-6050)
  delay(30);
}

void loop() {
  imu6050_data();
  
  accdata = String(x[0], 5) + ", " + String(y[0], 5) + ", " + String(z[0], 5);
  gyrdata = String(gx[0], 5) + ", " + String(gy[0], 5) + ", " + String(gz[0], 5);
  
  logdata = accdata + ", " + gyrdata;
  
  Serial.println(logdata);

}

void imu6050_data() {
  ID = MPU_addr;

  readmultiRegister(0x3B, 14);

  x[0] = values[0] << 8 | values[1];
  y[0] = values[2] << 8 | values[3];
  z[0] = values[4] << 8 | values[5];
  ta[10] = values[6] << 8 | values[7];
  gx[0] = values[8] << 8 | values[9];
  gy[0] = values[10] << 8 | values[11];
  gz[0] = values[12] << 8 | values[13];

  if (x[0] >= 0x8000) {
    x[0] = x[0] -  0x10000;
  }
  if (y[0] >= 0x8000) {
    y[0] = y[0] -  0x10000;
  }
  if (z[0] >= 0x8000) {
    z[0] = z[0] -  0x10000;
  }  
  if (gx[0] >= 0x8000)
    gx[0] = gx[0] -  0x10000;
  if (gy[0] >= 0x8000)
    gy[0] = gy[0] -  0x10000;
  if (gz[0] >= 0x8000)
    gz[0] = gz[0] -  0x10000;

  x[0] = x[0] / 0x4000;
  y[0] = y[0] / 0x4000;
  z[0] = z[0] / 0x4000;
  
  gx[0] = gx[0] / 131;
  gy[0] = gy[0] / 131;
  gz[0] = gz[0] / 131;

  ta[0] = (ta[10] / 340) - 160.87 + 3.63;
}
void writeRegister(int ID, int reg, int data)
{
  Wire.beginTransmission(ID);
  Wire.write(reg);
  Wire.write(data);
  Wire.endTransmission();
}

int readRegister(int reg)
{
  Wire.beginTransmission(ID);
  Wire.write(reg);
  Wire.endTransmission();
  Wire.requestFrom(ID, 1);
  if (Wire.available() <= 1)
  {
    return Wire.read();
  }
}

void readmultiRegister(int fst, int num)
{
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
