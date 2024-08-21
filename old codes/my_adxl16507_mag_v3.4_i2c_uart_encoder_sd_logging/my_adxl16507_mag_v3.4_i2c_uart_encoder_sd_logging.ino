/*log
  7/7 add hall
  7/9 發現頻率又降了?(319hz)
  7/12 換新SD卡 升至(339hz)
  2022/4/22 mag spi gr8
  2022/6/26 3.1 16475 give up back to 16507 and so upgr8
  2022/12/16 3.3 wire_v4 i2cmag、uart、IMU temp、fix timer_v2
*/

#include <SPI.h>
#include <ADIS16470.h>
#include <SD.h>
#include <Wire.h>
#include"register.h"
#include <SoftwareSerial.h>
#include <digitalWriteFast.h>

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
int32_t gxo, gyo, gzo, axo, ayo, azo, ito = 0;
double ax, ay, az, gx, gy, gz;
double ax2, ay2, az2, gx2, gy2, gz2;
byte regread0 , regread1, regread2, regread3;
String logFileName, accdata, gyrdata, magdata, logdata, mag2data, tmdata, halldata, flydata; // Rotordata;
File logFile;
const int beeper = 4;
const int beeper2 = 23;
byte IMU507;

const int mag_I2c = 0x30;
int CS;
int ID, delayy = 1000, setF = 100;
long values[10], t[12];
double x[3], y[3], z[3], xm[7], ym[7], zm[7], gx1, gy1, gz1, t0, timee = 0, f, f2, tm[7], tg; //, x1, y1, z1
int maxsize = 209715200;//209715200=200mb 262144000=250mb = 250*2^20;
unsigned long i = 0, t1, beepert;
const int H1 = 20, H2 = 21, H3 = 22; //hall need wire
int setpin = 9;
extern float tempmonGetTemp(void);
double tt[10];
long ii = 500; long iii = 1000;
double position;
double setd = 1.5;

ADIS16470 IMU(acc16475_CS, acc16475_DR, acc16475_RSET); // Chip Select, Data Ready, Reset Pin Assignments
ADIS16470 IMU2(acc16507_CS, acc16507_DR, acc16507_RSET); // Chip Select, Data Ready, Reset Pin Assignments

void setup() {
  Serial.begin(115200); // Initialize serial output via USB
  delay(100);
  Serial.println(F("Serial.begin"));
  Serial.println(F("SPI.begin"));
  SPI.begin();
  delay(100);

  Serial.println(F("Wire.begin"));
  Wire.begin();
  Wire.setClock(400000);

  Serial.println(F("HC12.begin"));
  //HC12.begin(115200);
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

  pinModeFast(H1, INPUT_PULLUP);
  pinModeFast(H2, INPUT_PULLUP);
  pinModeFast(H3, INPUT_PULLUP);
  pinMode(beeper2, OUTPUT);
  pinMode(beeper, OUTPUT);
  attachInterrupt(H1, phaseA, RISING);
  attachInterrupt(H2, phaseB, RISING);

  Serial.println(F("MMC5983MA setting"));
  //MMC5983MA n setting
  writeRegister(mag_I2c, MMC5983MA_INTERNAL_CONTROL_0, 0x16); //reseting //2022/3/17 add back
  delayMicroseconds(150);
  writeRegister(mag_I2c, MMC5983MA_INTERNAL_CONTROL_1, 0x03); //2 200hz、350hz //3 600hz
  delayMicroseconds(150);
  writeRegister(mag_I2c, MMC5983MA_INTERNAL_CONTROL_0, 0x01); //initiate measurement
  delayMicroseconds(150);

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

  //while (!SD.begin(SD_CS)) {
    Serial.println(F("Card failed, or not present"));
    digitalWrite(beeper, HIGH);
    delay(500);
    digitalWrite(beeper, LOW);
    delay(500);
  //}

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

  mag_data();  //12 15 back to my_adxl16507_mag_v2_sd_logging
  IMU.configSPI(); // Configure SPI communication  : SPISettings IMUSettings(1000000, MSBFIRST, SPI_MODE3);
  if (digitalReadFast(acc16475_DR) == HIGH) {
    getmydata();
  }
  if (digitalReadFast(acc16507_DR) == HIGH) {
    IMU507 = 1;
    getmydata();
    IMU507 = 0;
  }

  //halldata = String(analogRead(H1)) + ", " + String(analogRead(H2)) + ", " + String(analogRead(H3)) + ", " + String(analogRead(H4));
  halldata = String(position);
  accdata = String(ax, 7) + ", " + String(ay, 7) + ", " + String(az, 7) + ", " + String(ax2, 7) + ", " + String(ay2, 7) + ", " + String(az2, 7);
  gyrdata = String(gx, 6) + ", " + String(gy, 6) + ", " + String(gz, 6) + ", " + String(gx2, 6) + ", " + String(gy2, 6) + ", " + String(gz2, 6);

  //mag_data();  //12 15 back to my_adxl16507_mag_v2_sd_logging
  magdata = String(xm[1], 5) + ", " + String(ym[1], 5) + ", " + String(zm[1], 5);
  tmdata = String(tm[1], 1) + ", " + String(tm[3], 1) + ", " + String(tm[4], 1) + ", " + String(tempmonGetTemp(), 2);// + ", " + String(tm[2], 1)

  logdata = String(timee, 3) + ", " + accdata + "," + gyrdata + ", " + magdata + ", " + halldata + ", " + tmdata + ", " + String(f, 2) + ", " + String(delayy);
  //flydata = String(timee, 3) + ", " + String(ax, 7) + ", " + String(ay, 7) + ", " + String(az, 7) + ", " + String(gx, 6) + ", " + String(gy, 6) + ", " + String(gz, 6) + ", " + String(xm[1], 5) + ", " + String(ym[1], 5) + ", " + String(zm[1], 5);
  flydata = String(timee, 3) + "," + String(ax, 7) + "," + String(ay, 3) + "," + String(az, 3) + "," + String(gx, 6) + "," + String(gy, 6) + "," + String(gz, 6) + "," + String(xm[1], 5) + "," + String(ym[1], 3) + "," + String(zm[1], 3) + "," + String(tempmonGetTemp(), 2) + "," + String(position) + "," + String(f, 1);
  Serial.println(logdata);
  //Serial.println(flydata);
  //Serial.println(halldata);
  //Serial.println(magdata);

  HC12.println(flydata);

  if (!logFile) {
    logFile = SD.open(logFileName, FILE_WRITE);
  }

  if (logFile) {
    logFile.println(logdata);

    if (t0 >= 20) {
      if (i % 500 == 0) { //v2
        digitalWriteFast(beeper, HIGH);
        beepert = i;
      }
      if (beepert + 10 == i) {
        digitalWriteFast(beeper, LOW);
      }
    }
  }
  else {
    Serial.println("error opening test.txt");
  }

  if (gx == 0) {
    digitalWriteFast(beeper, HIGH);
  }


  //timer
  timer_v2();

  delayMicroseconds(delayy);
}

void mag_data() {
  ID = mag_I2c;

  writeRegister(mag_I2c, MMC5983MA_INTERNAL_CONTROL_0, 0x10); //get t,m data //only tm 0x02//set 0x10
  delayMicroseconds(30);
  tm[0] = readRegister(MMC5983MA_TOUT);
  delayMicroseconds(30);

  writeRegister(mag_I2c, MMC5983MA_INTERNAL_CONTROL_0, 0x01);
  delayMicroseconds(30);
  readmultiRegister (MMC5983MA_XOUT_0, 7);


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

  xm[1] = xm[0];
  ym[1] = ym[0];
  zm[1] = zm[0];
  tm[1] = tm[0] - 0;//temp m calibrate//-18.5 //16.4
}

void getmydata() {
  if (IMU507 == 1) {
    gxo = (IMU2.regRead(X_GYRO_OUT) << 16) + IMU2.regRead(X_GYRO_LOW);
    gyo = (IMU2.regRead(Y_GYRO_OUT) << 16) + IMU2.regRead(Y_GYRO_LOW);
    gzo = (IMU2.regRead(Z_GYRO_OUT) << 16) + IMU2.regRead(Z_GYRO_LOW);
    axo = (IMU2.regRead(X_ACCL_OUT) << 16) + IMU2.regRead(X_ACCL_LOW);
    ayo = (IMU2.regRead(Y_ACCL_OUT) << 16) + IMU2.regRead(Y_ACCL_LOW);
    azo = (IMU2.regRead(Z_ACCL_OUT) << 16) + IMU2.regRead(Z_ACCL_LOW);
    azo = (IMU2.regRead(Z_ACCL_OUT) << 16) + IMU2.regRead(Z_ACCL_LOW);
    ito = IMU2.regRead(TEMP_OUT);
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
    tm[4] = ito;
    tm[4] = (tm[4] * 0.1) - 0; // temp m calibrate
  }
  else {
    gxo = (IMU.regRead(X_GYRO_OUT) << 16) + IMU.regRead(X_GYRO_LOW);
    gyo = (IMU.regRead(Y_GYRO_OUT) << 16) + IMU.regRead(Y_GYRO_LOW);
    gzo = (IMU.regRead(Z_GYRO_OUT) << 16) + IMU.regRead(Z_GYRO_LOW);
    axo = (IMU.regRead(X_ACCL_OUT) << 16) + IMU.regRead(X_ACCL_LOW);
    ayo = (IMU.regRead(Y_ACCL_OUT) << 16) + IMU.regRead(Y_ACCL_LOW);
    azo = (IMU.regRead(Z_ACCL_OUT) << 16) + IMU.regRead(Z_ACCL_LOW);
    ito = IMU.regRead(TEMP_OUT);
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
    tm[3] = ito;
    tm[3] = (tm[3] * 0.1) - 0; // temp m calibrate
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
  digitalWriteFast(CS, LOW);
  SPI.transfer(thisRegister);
  SPI.transfer(value);
  digitalWriteFast(CS, HIGH);
}

byte sreadRegister (byte thisRegister) {
  byte inByte = 0 ;
  digitalWriteFast(CS, LOW);
  SPI.transfer(thisRegister | 0x80);
  inByte = SPI.transfer(0x00);
  digitalWriteFast(CS, HIGH);
  return inByte;
}

void sreadmultiRegister(byte thisRegister, int num) {
  int k = 0;
  memset(values, 0, sizeof(values));
  digitalWriteFast(CS, LOW);
  SPI.transfer(thisRegister | 0x80);
  while (k < num)
  {
    values[k++] = SPI.transfer(0x00);
  }
  digitalWriteFast(CS, HIGH);
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

void phaseA() {
  if (digitalReadFast(H1) == HIGH) {
    if (digitalReadFast(H2) == LOW) {
      position = position + setd;
    } else {
      position = position - setd;
    }
  }
  if (digitalReadFast(H1) == LOW) {
    if (digitalReadFast(H2) == HIGH) {
      position = position + setd;
    } else {
      position = position - setd;
    }
  }
  if (position == -setd) {
    position = 360 - setd;
  }
  if (position == 360) {
    position = 0;
  }
}

void phaseB() {
  if (digitalReadFast(H2) == HIGH) {
    if (digitalReadFast(H1) == HIGH) {
      position = position + setd;
    } else {
      position = position - setd;
    }
  }
  if (digitalReadFast(H2) == LOW) {
    if (digitalReadFast(H1) == LOW) {
      position = position + setd;
    } else {
      position = position - setd;
    }
  }
  if (position == -setd) {
    position = 360 - setd;
  }
  if (position == 360) {
    position = 0;
  }
}



void timer_v2() {
  i++;
  timee = millis() * 0.001;
  f2 = i / timee;
  /*Serial.println(String(timee, 3));
    Serial.println(i);
    Serial.println(ii);
    Serial.println(iii);
    Serial.println(tt[3]);
    Serial.println(tt[1]);
    Serial.println(tt[2]);
    Serial.println(f);
    Serial.println(f2);
    Serial.println(delayy);*/

  if (i >= iii) {
    iii = i + 500;
    tt[1] = timee;
    tt[3] = tt[0];
    tt[2] = tt[1] - tt[0];
    f = 500 / tt[2];

    if (abs(f - setF) > 1) {
      if (setF > f) {
        delayy -= (setF - f) * 100;
      }
      if (setF < f) {
        delayy += (f - setF) * 100;
      }
      if (delayy > 10000) {
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

  if (i % 12000 == 0) { //設每幾筆資料儲存至SD卡
    if (logFile.size() > maxsize) {
      logFileName = nextLogFile();
    }
    logFile.close(); // close the file
  }
}
