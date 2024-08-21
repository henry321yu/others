/*log
   7/7 add hall
   7/9 發現頻率又降了?(319hz)
   7/12 換新SD卡 升至(339hz)
   2022/4/22 mag spi gr8
*/

#include <SPI.h>
#include <ADIS16470.h>
#include <SD.h>
#include <Wire.h>
#include"register.h"
#include <SoftwareSerial.h>

const int SD_CS = 10;
const int acc16475_CS = 15;
const int acc16475_DR = 2;
const int acc16475_RSET = 3;
const int acc16507_CS = 14;
const int acc16507_DR = 16;
const int acc16507_RSET = 17;
int16_t regData[32];
byte accdataN = 24;
int32_t gxo, gyo, gzo, axo, ayo, azo = 0;
int32_t gxb, gyb, gzb, axb, ayb, azb = 0;
double ax, ay, az, gx, gy, gz;
double ax2, ay2, az2, gx2, gy2, gz2;
byte regread0 , regread1, regread2, regread3;
String logFileName, accdata, gyrdata, magdata, logdata, mag2data, tmdata, halldata, flydata; // Rotordata;
File logFile;
const int beeper = 4;
byte IMU507;

const int mag1CS = 5;
const int mag2CS = 6;
int CS;
int ID, mn[2] = {5, 6}, delayy = 0, setF = 100;
long values[10], t[12];
double x[3], y[3], z[3], xm[7], ym[7], zm[7], gx1, gy1, gz1, t0, timee = 0, f, tm[7], tg; //, x1, y1, z1
int maxsize = 262144000;//250mb = 250*2^20;
unsigned long i = 0, t1, beepert;
const int H1 = A9, H2 = A8; //hall need wire
int setpin = 9;

void setup() {
  Serial.begin(115200); // Initialize serial output via USB
  SPI.begin();

  pinMode(H1, INPUT);
  pinMode(H2, INPUT);
  pinMode(beeper, OUTPUT);
  pinMode(mag1CS, OUTPUT);
  pinMode(mag1CS, OUTPUT);
  digitalWrite(mag1CS, HIGH);
  digitalWrite(mag2CS, HIGH);

  //MMC5983MA n setting
  for (int j = 0; j < 2; j++) {
    CS = mn[j];
    swriteRegister(MMC5983MA_INTERNAL_CONTROL_0, 0x16); //reseting //2022/3/17 add back
    delayMicroseconds(150);

    swriteRegister(MMC5983MA_INTERNAL_CONTROL_1, 0x03); //2 200hz、350hz //3 600hz
    delayMicroseconds(150);
    swriteRegister(MMC5983MA_INTERNAL_CONTROL_0, 0x01); //initiate measurement
    delayMicroseconds(150);
  }



  delay(2000);
}


void loop() {
  t0 = millis() * 0.001;
  mag_data_test();
  magdata = String(t0, 3) + ", " +  String(xm[1], 5) + ", " + String(ym[1], 5) + ", " + String(zm[1], 5) + ", " + String(xm[2], 5) + ", " + String(ym[2], 5) + ", " + String(zm[2], 5) + ", " + String(tm[0], 3);
  Serial.println(magdata);
  delayMicroseconds(100000);
}

void mag_data_test() {
  for (int j = 0; j < 2; j++) {
    CS = mn[j];
    swriteRegister(MMC5983MA_INTERNAL_CONTROL_0, 0x10); //get t,m data //only tm 0x02//set 0x10
    delayMicroseconds(1500);
    tm[0] = sreadRegister(MMC5983MA_TOUT);

    swriteRegister(MMC5983MA_INTERNAL_CONTROL_0, 0x01);
    delayMicroseconds(1500);
    sreadmultiRegister (MMC5983MA_XOUT_0, 7);

    delayMicroseconds(1500);
    //Serial.println(xm[0]);

    t[3] = values[0] << 10 | values[1] << 2 | bitRead(values[6], 7) << 1 | bitRead(values[6], 6);  //18bit
    t[4] = values[2] << 10 | values[3] << 2 | bitRead(values[6], 5) << 1 | bitRead(values[6], 4);
    t[5] = values[4] << 10 | values[5] << 2 | bitRead(values[6], 3) << 1 | bitRead(values[6], 2);

    xm[0] = t[3];
    ym[0] = t[4];
    zm[0] = t[5];

    xm[0] = (xm[0] - 0x1FFFF) / 163.84;  //18bit
    ym[0] = (ym[0] - 0x1FFFF) / 163.84;
    zm[0] = (zm[0] - 0x1FFFF) / 163.84;

    tm[0] -= 75;
    tm[0] = tm[0] * 0.7;

    xm[j + 1] = xm[0];
    ym[j + 1] = ym[0];
    zm[j + 1] = zm[0];
    switch (j) {
      case 0:
        tm[j + 1] = tm[0] - 16.4; break;
      case 1:
        tm[j + 1] = tm[0] - 15; break;//-18.5
    }
  }
}

void swriteRegister (byte thisRegister, byte value) { //spi communication
  digitalWrite(CS, LOW);
  SPI.transfer(thisRegister);
  SPI.transfer(value);
  digitalWrite(CS, HIGH);
}

byte sreadRegister (byte thisRegister) {
  byte inByte = 0 ;
  digitalWrite(CS, LOW);
  SPI.transfer(thisRegister | 0x80);
  inByte = SPI.transfer(0x00);
  digitalWrite(CS, HIGH);
  return inByte;
}

void sreadmultiRegister(byte thisRegister, int num) {
  int k = 0;
  memset(values, 0, sizeof(values));
  digitalWrite(CS, LOW);
  SPI.transfer(thisRegister | 0x80);
  while (k < num)
  {
    values[k++] = SPI.transfer(0x00);
  }
  digitalWrite(CS, HIGH);
}
