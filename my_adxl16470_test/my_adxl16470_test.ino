/*log
   7/7 add hall
   7/9 發現頻率又降了?(319hz)
   7/12 換新SD卡 升至(339hz)
*/

#include <SPI.h>
#include <ADIS16470.h>
#include <SD.h>
#include <Wire.h>
#define TCAADDR 0x70
#include"register.h"

const int SD_CS = 10;
const int acc16475_CS = 10;
const int acc16475_DR = 3;
const int acc16475_RSET = 4;
int16_t regData[32];
byte accdataN = 24;
int32_t gxo, gyo, gzo, axo, ayo, azo = 0;
int32_t gxb, gyb, gzb, axb, ayb, azb = 0;
double ax, ay, az, gx, gy, gz;
byte regread0 , regread1, regread2, regread3;
String logFileName, accdata, gyrdata, magdata, logdata, mag2data, tmdata, halldata;// Rotordata;
File logFile;
const int beeper = 4;


int ID, mn[6] = {2, 3, 4, 5, 6, 7}, delayy = 0, setF = 100;
long values[10], t[12];
double x[3], y[3], z[3], xm[7], ym[7], zm[7], gx1, gy1, gz1, t0, timee = 0, f, tm[7], tg; //, x1, y1, z1
int maxsize = 262144000;//250mb = 250*2^20;
unsigned long i = 0, t1, beepert;



ADIS16470 IMU(acc16475_CS, acc16475_DR, acc16475_RSET); // Chip Select, Data Ready, Reset Pin Assignments

void setup() {
  Serial.begin(115200); // Initialize serial output via USB
  //IMU.configSPI(); // Configure SPI communication
  Wire.begin();
  Wire.setClock(400000);

  IMU.regWrite(MSC_CTRL, 0xC1);  // Enable Data Ready, set polarity
  delay(30);
  //IMU.regWrite(FILT_CTRL, 0x04); // Set digital filter
  //IMU.regWrite(DEC_RATE, 0x03); // Disable decimation
  IMU.regWrite(FILT_CTRL, 0x00); // Set digital filter
  delay(30);
  IMU.regWrite(DEC_RATE, 0x00); // Disable decimation
  delay(30);
  //IMU.regWrite(NULL_CFG, 0x3F0A); //  bias estimation period  deafult 0x70A unenble 0xA all enble 3F0A
  //IMU.regWrite(GLOB_CMD, 0x01); // Bias Correction Update

  delay(30);

  Serial.println(F("done initialize"));

  delay(1000);
  //IMU.configSPI(); // Configure SPI communication


  digitalWrite(beeper, HIGH);
  delay(100);
  digitalWrite(beeper, LOW);
  delay(500);
  digitalWrite(beeper, HIGH);
  delay(100);
  digitalWrite(beeper, LOW);
  delay(2000);
}


void loop() {
  t0 = millis() * 0.001;
  IMU.configSPI(); // Configure SPI communication
  if (digitalRead(acc16475_DR) == HIGH) {
    getmydata();
  }
  accdata = String(ax, 7) + ", " + String(ay, 7) + ", " + String(az, 7);
  gyrdata = String(gx, 6) + ", " + String(gy, 6) + ", " + String(gz, 6);

  tmdata = String(tm[1], 2) + ", " + String(tm[2], 2);

  logdata = accdata + "," + gyrdata + ", " + String(f, 2);
  Serial.println(logdata);
  //Serial.println( halldata);


  //timer
  if (t0 >= 10) {
    if (i == 0) {
      t1 = millis();
    }
    i++;
    timer();
  }

  delayMicroseconds(80); //120 teensy 4.0 600mhz 350hz 16475,2*mag,sd   一樣code,但降幀,原400,後320,現調至350
}

void getmydata() {
  gxo = (IMU.regRead(X_GYRO_OUT) << 16) + IMU.regRead(X_GYRO_LOW);
  gyo = (IMU.regRead(Y_GYRO_OUT) << 16) + IMU.regRead(Y_GYRO_LOW);
  gzo = (IMU.regRead(Z_GYRO_OUT) << 16) + IMU.regRead(Z_GYRO_LOW);
  axo = (IMU.regRead(X_ACCL_OUT) << 16) + IMU.regRead(X_ACCL_LOW);
  ayo = (IMU.regRead(Y_ACCL_OUT) << 16) + IMU.regRead(Y_ACCL_LOW);
  azo = (IMU.regRead(Z_ACCL_OUT) << 16) + IMU.regRead(Z_ACCL_LOW);

  gx = gxo;
  gy = gyo;
  gz = gzo;
  gx = gx / 10485760 ;
  gy = gy / 10485760 ;
  gz = gz / 10485760 ;
  ax = axo;
  ay = ayo;
  az = azo;
  ax = ax / 0xFA00000;// * 9.80665;
  ay = ay / 0xFA00000;// * 9.80665;
  az = az / 0xFA00000;// * 9.80665;
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


void timer() {
  timee = (millis() - t1) * 0.001;
  f = i / timee;
  if (f >= 1000) { // fix ,999999
    f = 0;
  }
}
