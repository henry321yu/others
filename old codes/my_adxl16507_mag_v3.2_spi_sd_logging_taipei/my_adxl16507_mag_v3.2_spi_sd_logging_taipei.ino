/*log
  7/7 add hall
  7/9 發現頻率又降了?(319hz)
  7/12 換新SD卡 升至(339hz)
  2022/4/22 mag spi gr8
  2022/6/26 3.1 16475 give up back to 16507 and so upgr8
*/

#include <SPI.h>
#include <ADIS16470.h>
#include <SD.h>
#include <Wire.h>
#include"register.h"
#include <SoftwareSerial.h>

SoftwareSerial HC12(7, 8);
//SoftwareSerial HC12(21, 20);

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
int ID, mn[2] = {5, 6}, delayy = 10000, setF = 100;
long values[10], t[12];
double x[3], y[3], z[3], xm[7], ym[7], zm[7], gx1, gy1, gz1, t0, timee = 0, f, tm[7], tg; //, x1, y1, z1
int maxsize = 209715200;//209715200=200mb 262144000=250mb = 250*2^20;
unsigned long i = 0, t1, beepert;
const int H1 = A9, H2 = A8; //hall need wire
int setpin = 9;
extern float tempmonGetTemp(void);

ADIS16470 IMU(acc16475_CS, acc16475_DR, acc16475_RSET); // Chip Select, Data Ready, Reset Pin Assignments
ADIS16470 IMU2(acc16507_CS, acc16507_DR, acc16507_RSET); // Chip Select, Data Ready, Reset Pin Assignments

void setup() {
  Serial.begin(115200); // Initialize serial output via USB
  delay(100);
  Serial.println(F("Serial.begin"));
  Serial.println(F("SPI.begin"));
  SPI.begin();
  delay(100);

  Serial.println(F("HC12.begin"));
  HC12.begin(115200);
  delay(100);
  pinMode(setpin, OUTPUT);
  digitalWrite(setpin, LOW);
  delay(100);
  HC12.print("AT+B115200");
  delay(100);
  HC12.print("AT+C127");
  delay(100);
  HC12.print("AT+P8");
  delay(100);
  digitalWrite(setpin, HIGH);

  pinMode(H1, INPUT);
  pinMode(H2, INPUT);
  pinMode(beeper, OUTPUT);
  pinMode(mag1CS, OUTPUT);
  pinMode(mag2CS, OUTPUT);
  digitalWrite(mag1CS, HIGH);
  digitalWrite(mag2CS, HIGH);

  Serial.println(F("MMC5983MA setting"));
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

  Serial.println(F("ADIS16507 setting"));
  IMU.configSPI();
  //IMU setting
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


  IMU2.regWrite(MSC_CTRL, 0xC1);  // Enable Data Ready, set polarity
  delay(30);
  IMU2.regWrite(FILT_CTRL, 0x00); // Set digital filter
  delay(30);
  IMU2.regWrite(DEC_RATE, 0x00); // Disable decimation
  delay(30);

  Serial.println(F("SD.begin"));
  if (!SD.begin(SD_CS)) {
    Serial.println(F("Card failed, or not present"));
  }

  while (!SD.begin(SD_CS)) {
    Serial.println(F("Card failed, or not present"));
    digitalWrite(beeper, HIGH);
    delay(500);
    digitalWrite(beeper, LOW);
    delay(500);
  }

  logFileName = nextLogFile();
  logFile = SD.open(logFileName, FILE_WRITE);
  if (logFile) {
    //Serial.println(F("writing"));
  }
  // if the file didn't open, print an error:
  else {
    Serial.println(F("error opening file"));
    //return;
  }
  while (HC12.available()) {
    Serial.write(HC12.read());
  }


  Serial.println(F("done initialize"));
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

  mag_data_SPI();  //5 27

  IMU.configSPI(); // Configure SPI communication  : SPISettings IMUSettings(1000000, MSBFIRST, SPI_MODE3);
  if (digitalRead(acc16475_DR) == HIGH) {
    getmydata();
  }
  if (digitalRead(acc16507_DR) == HIGH) {
    IMU507 = 1;
    getmydata();
    IMU507 = 0;
  }

  halldata = String(analogRead(H1)) + ", " + String(analogRead(H2));
  accdata = String(ax, 7) + ", " + String(ay, 7) + ", " + String(az, 7) + ", " + String(ax2, 7) + ", " + String(ay2, 7) + ", " + String(az2, 7);
  gyrdata = String(gx, 6) + ", " + String(gy, 6) + ", " + String(gz, 6) + ", " + String(gx2, 6) + ", " + String(gy2, 6) + ", " + String(gz2, 6);

  //mag_data_SPI();
  magdata = String(xm[1], 5) + ", " + String(ym[1], 5) + ", " + String(zm[1], 5) + ", " + String(xm[2], 5) + ", " + String(ym[2], 5) + ", " + String(zm[2], 5);
  tmdata = String(tm[1], 2) + ", " + String(tm[2], 2) + ", " + String(tempmonGetTemp(), 2);

  logdata = String(timee, 3) + ", " + accdata + "," + gyrdata + ", " + magdata + ", " + halldata + ", " + tmdata + ", " + String(f, 2) + ", " + String(delayy);
  //flydata = String(timee, 3) + ", " + String(ax, 7) + ", " + String(ay, 7) + ", " + String(az, 7) + ", " + String(gx, 6) + ", " + String(gy, 6) + ", " + String(gz, 6) + ", " + String(xm[2], 5) + ", " + String(ym[2], 5) + ", " + String(zm[2], 5);
  flydata = String(timee, 3) + ", " + String(ax, 7) + ", " + String(ay, 7) + ", " + String(az, 7) + ", " + String(gx, 6) + ", " + String(gy, 6) + ", " + String(gz, 6) + ", " + String(xm[1], 5) + ", " + String(ym[1], 5) + ", " + String(zm[1], 5);
  //Serial.println(logdata);
  //Serial.println(flydata);
  //Serial.println(halldata);
  Serial.println(magdata);

  HC12.println(flydata);

  if (!logFile) {
    logFile = SD.open(logFileName, FILE_WRITE);
  }

  if (logFile) {
    logFile.println(logdata);

    if (t0 >= 20) {
      if (i % 500 == 0) { //v2
        digitalWrite(beeper, HIGH);
        beepert = i;
      }
      if (beepert + 10 == i) {
        digitalWrite(beeper, LOW);
      }
    }
  }
  else {
    Serial.println("error opening test.txt");
  }

  if (gx == 0) {
    digitalWrite(beeper, HIGH);
  }


  //timer
  if (t0 >= 30) {
    if (i == 0) {
      t1 = millis();
    }
    i++;
    timer_v2();
  }
  delayMicroseconds(delayy);
}

void mag_data_SPI() {
  for (int j = 0; j < 2; j++) {
    CS = mn[j];
    swriteRegister(MMC5983MA_INTERNAL_CONTROL_0, 0x10); //get t,m data //only tm 0x02//set 0x10
    delayMicroseconds(30);
    tm[0] = sreadRegister(MMC5983MA_TOUT);
    delayMicroseconds(30);

    swriteRegister(MMC5983MA_INTERNAL_CONTROL_0, 0x01);
    delayMicroseconds(30);
    sreadmultiRegister (MMC5983MA_XOUT_0, 7);


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
    gx2 = gx2 / 0x280000 ; //gx = gx / 0x280000 ; 16507
    gy2 = gy2 / 0x280000 ; //gx = gx / 0xA00000 ; 16475
    gz2 = gz2 / 0x280000 ;
    ax2 = axo;
    ay2 = ayo;
    az2 = azo;
    ax2 = ax2 / 5351254 / 9.80665; //ax = ax / 5351254 / 9.80665; 16507
    ay2 = ay2 / 5351254 / 9.80665; //ax = ax / 0xFA00000;// * 9.80665; 16475
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


String nextLogFile(void) {
  String filename;
  int logn = 0;
  for (int i = 0; i < 999; i++) {
    // Construct a file with PREFIX[Index].SUFFIX
    filename = String("log") + String(logn) + String(".csv");
    // If the file name doesn't exist, return it
    if (!SD.exists(filename))    {
      return filename;
    }
    // Otherwise increment the index, and try again
    logn++;
  }
  return "";
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

void timer_v2() {
  timee = (millis() - t1) * 0.001;

  if (i > iii) {
    iii = iii + 500;
    tt[1] = timee;
    tt[2] = tt[1] - tt[0];
    f = 500 / tt[2];
    if (abs(f - setF) > 1) {
      if (setF > f) {
        delayy -= (setF - f) * 100;
      }
      if (setF < f) {
        delayy += (f - setF) * 100;
      }
      if (delayy > 20000) {
        delayy = 10000;
      }
      if (delayy < 0) {
        delayy = 0;
      }
    }
  }
  if (i > ii) {
    ii = ii + 500;
    tt[0] = timee;
  }

  if (i % 12000 == 0) { //設每幾筆資料儲存至SD卡 100約占0.03秒
    if (logFile.size() > maxsize) {
      logFileName = nextLogFile();
    }
    logFile.close(); // close the file
  }
}
