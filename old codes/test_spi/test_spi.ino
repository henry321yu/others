#include <SPI.h>
#include <ADIS16470.h>
#include"register.h"

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
const int beeper = 4;
byte IMU507;
int CS = 5;


const int mag_I2c = 0x30; //
int ID, mn[6] = {2, 3, 4, 5, 6, 7}, delayy = 0, setF = 100;
long values[10], t[12];
double x[3], y[3], z[3], xm[7], ym[7], zm[7], gx1, gy1, gz1, t0, timee = 0, f, tm[7], tg; //, x1, y1, z1
int maxsize = 262144000;//250mb = 250*2^20;
unsigned long i = 0, t1, beepert;
const int H1 = A9, H2 = A8; //hall need wire
int setpin = 9;



ADIS16470 IMU(acc16475_CS, acc16475_DR, acc16475_RSET); // Chip Select, Data Ready, Reset Pin Assignments
ADIS16470 IMU2(acc16507_CS, acc16507_DR, acc16507_RSET); // Chip Select, Data Ready, Reset Pin Assignments


void setup() {
  SPI.begin();
  SPISettings IMUSettings(1000000, MSBFIRST, SPI_MODE3); // original 1000000
  SPI.beginTransaction(IMUSettings);

  pinMode(H1, INPUT);
  pinMode(H2, INPUT);
  pinMode(beeper, OUTPUT);

  //IMU setting
  IMU.regWrite(MSC_CTRL, 0xC1);  // Enable Data Ready, set polarity
  delayMicroseconds(100);
  //IMU.regWrite(FILT_CTRL, 0x04); // Set digital filter
  //IMU.regWrite(DEC_RATE, 0x03); // Disable decimation
  IMU.regWrite(FILT_CTRL, 0x00); // Set digital filter
  delayMicroseconds(100);
  IMU.regWrite(DEC_RATE, 0x00); // Disable decimation
  delayMicroseconds(100);
  //IMU.regWrite(NULL_CFG, 0x3F0A); //  bias estimation period  deafult 0x70A unenble 0xA all enble 3F0A
  //IMU.regWrite(GLOB_CMD, 0x01); // Bias Correction Update


  IMU2.regWrite(MSC_CTRL, 0xC1);  // Enable Data Ready, set polarity
  delayMicroseconds(100);
  IMU2.regWrite(FILT_CTRL, 0x00); // Set digital filter
  delayMicroseconds(100);
  IMU2.regWrite(DEC_RATE, 0x00); // Disable decimation
  delayMicroseconds(100);


  delay(2000);
}


void loop() {
  t0 = millis() * 0.001;
  IMU.configSPI(); // Configure SPI communication
  if (digitalRead(acc16475_DR) == HIGH) {
    getmydata();
  }
  if (digitalRead(acc16507_DR) == HIGH) {
    IMU507 = 1;
    getmydata();
    IMU507 = 0;
  }

  accdata = String(ax, 7) + ", " + String(ay, 7) + ", " + String(az, 7) + ", " + String(ax2, 7) + ", " + String(ay2, 7) + ", " + String(az2, 7);
  gyrdata = String(gx, 6) + ", " + String(gy, 6) + ", " + String(gz, 6) + ", " + String(gx2, 6) + ", " + String(gy2, 6) + ", " + String(gz2, 6);
  Serial.println(accdata);

  delayMicroseconds(100000); //120 teensy 4.0 600mhz 350hz 16475,2*mag,sd   一樣code,但降幀,原400,後320,現調至350
}


void getmydata() {
  if (IMU507 == 1) {
    gxo = (IMU2.regRead(X_GYRO_OUT) << 16) + IMU2.regRead(X_GYRO_LOW);
    gyo = (IMU2.regRead(Y_GYRO_OUT) << 16) + IMU2.regRead(Y_GYRO_LOW);
    gzo = (IMU2.regRead(Z_GYRO_OUT) << 16) + IMU2.regRead(Z_GYRO_LOW);
    axo = (IMU2.regRead(X_ACCL_OUT) << 16) + IMU2.regRead(X_ACCL_LOW);
    ayo = (IMU2.regRead(Y_ACCL_OUT) << 16) + IMU2.regRead(Y_ACCL_LOW);
    azo = (IMU2.regRead(Z_ACCL_OUT) << 16) + IMU2.regRead(Z_ACCL_LOW);
    gx2 = gxo;
    gy2 = gyo;
    gz2 = gzo;
    gx2 = gx2 / 0x280000 ;
    gy2 = gy2 / 0x280000 ;
    gz2 = gz2 / 0x280000 ;
    ax2 = axo;
    ay2 = ayo;
    az2 = azo;
    ax2 = ax2 / 5351254 / 9.80665;
    ay2 = ay2 / 5351254 / 9.80665;
    az2 = az2 / 5351254 / 9.80665;
  }
  else {
    gxo = (IMU.regRead(X_GYRO_OUT) << 16) + IMU.regRead(X_GYRO_LOW);
    gyo = (IMU.regRead(Y_GYRO_OUT) << 16) + IMU.regRead(Y_GYRO_LOW);
    gzo = (IMU.regRead(Z_GYRO_OUT) << 16) + IMU.regRead(Z_GYRO_LOW);
    axo = (IMU.regRead(X_ACCL_OUT) << 16) + IMU.regRead(X_ACCL_LOW);
    ayo = (IMU.regRead(Y_ACCL_OUT) << 16) + IMU.regRead(Y_ACCL_LOW);
    azo = (IMU.regRead(Z_ACCL_OUT) << 16) + IMU.regRead(Z_ACCL_LOW);
    gx = gxo;
    gy = gyo;
    gz = gzo;
    gx = gx / 0x280000 ;
    gy = gy / 0x280000 ;
    gz = gz / 0x280000 ;
    ax = axo;
    ay = ayo;
    az = azo;
    ax = ax / 5351254 / 9.80665;
    ay = ay / 5351254 / 9.80665;
    az = az / 5351254 / 9.80665;
  }
}
