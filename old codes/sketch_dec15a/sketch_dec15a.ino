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
#include <SoftwareSerial.h>

SoftwareSerial HC12(7, 8);

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


const int mag_I2c = 0x30; //
int ID, mn[6] = {2, 3, 4, 5, 6, 7}, delayy = 0, setF = 100;
long values[10], t[12];
double x[3], y[3], z[3], xm[7], ym[7], zm[7], gx1, gy1, gz1, t0, timee = 0, f, tm[7], tg; //, x1, y1, z1
int maxsize = 262144000;//250mb = 250*2^20;
unsigned long i = 0, t1, beepert;
const int H1 = A9, H2 = A8; //hall need wire
int setpin=9;



ADIS16470 IMU(acc16475_CS, acc16475_DR, acc16475_RSET); // Chip Select, Data Ready, Reset Pin Assignments
ADIS16470 IMU2(acc16507_CS, acc16507_DR, acc16507_RSET); // Chip Select, Data Ready, Reset Pin Assignments

void setup() {
  Serial.begin(115200); // Initialize serial output via USB
  //IMU.configSPI(); // Configure SPI communication
  Wire.begin();
  Wire.setClock(400000);

  HC12.begin(115200);
  delay(1000);
  pinMode(setpin, OUTPUT);
  digitalWrite(setpin, LOW);
  delay(1000);
  HC12.print("AT+B115200");
  digitalWrite(setpin, HIGH);

  pinMode(H1, INPUT);
  pinMode(H2, INPUT);
  pinMode(beeper, OUTPUT);

  if (!SD.begin(SD_CS)) {
    Serial.println(F("Card failed, or not present"));
  }

  Serial.println(F("Card failed, or not present"));
  digitalWrite(beeper, HIGH);
  delay(500);
  digitalWrite(beeper, LOW);
  delay(500);

  //MMC5983MA n setting
  for (int j = 0; j < 2; j++) {
    tcaselect(mn[j]);
    writeRegister(mag_I2c, MMC5983MA_INTERNAL_CONTROL_0, 0x16); //reseting //2022/3/17 add back
    delay(30);

    writeRegister(mag_I2c , MMC5983MA_INTERNAL_CONTROL_1, 0x03); //2 200hz、350hz //3 600hz
    delay(30);
    writeRegister(mag_I2c , MMC5983MA_INTERNAL_CONTROL_0, 0x01); //initiate measurement
    delay(30);
  }

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


  delay(30);

  logFileName = nextLogFile();
  //Serial.println(F("done initialize"));

  delay(500);
  logFile = SD.open(logFileName, FILE_WRITE);
  if (logFile) {
    //Serial.println(F("writing"));
  }
  // if the file didn't open, print an error:
  else {
    Serial.println(F("error opening file"));
    //return;
  }
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
  if (digitalRead(acc16507_DR) == HIGH) {
    IMU507 = 1;
    getmydata();
    IMU507 = 0;
  }

  halldata = String(analogRead(H1)) + ", " + String(analogRead(H2));
  //halldata = String(analogRead(H2));
  accdata = String(ax, 7) + ", " + String(ay, 7) + ", " + String(az, 7) + ", " + String(ax2, 7) + ", " + String(ay2, 7) + ", " + String(az2, 7);
  gyrdata = String(gx, 6) + ", " + String(gy, 6) + ", " + String(gz, 6) + ", " + String(gx2, 6) + ", " + String(gy2, 6) + ", " + String(gz2, 6);

  mag_data();
  magdata = String(xm[1], 5) + ", " + String(ym[1], 5) + ", " + String(zm[1], 5) + ", " + String(xm[2], 5) + ", " + String(ym[2], 5) + ", " + String(zm[2], 5) + ", " + String((xm[1] + xm[2]), 5) + ", " + String((ym[1] + ym[2]), 5) + ", " + String((zm[1] + zm[2]), 5);
  tmdata = String(tm[1], 2) + ", " + String(tm[2], 2);

  logdata = String(timee, 3) + ", " + accdata + "," + gyrdata + ", " + magdata + ", " + halldata + ", " + tmdata + ", " + String(f, 2);
  flydata = String(timee, 3) + ", " + String(ax, 7) + ", " + String(ay, 7) + ", " + String(az, 7) + ", " + String(gx, 6) + ", " + String(gy, 6) + ", " + String(gz, 6) + ", " + String(xm[2], 5) + ", " + String(ym[2], 5) + ", " + String(zm[2], 5);
  Serial.println(logdata);
  //Serial.println(flydata);
  //Serial.println(halldata);
  //Serial.println(magdata);
  //HC12.print(flydata);
  HC12.print(logdata);
  HC12.write("\n");

  if (!logFile) {
    logFile = SD.open(logFileName, FILE_WRITE);
        digitalWrite(beeper, LOW);
  }
  if (logFile) {
    logFile.println(logdata);

    if (t0 >= 20) {
      //if (i % 2000 == 0) { //v2
        digitalWrite(beeper, HIGH);
        //beepert = i;
      //}
      //if (beepert + 10 == i) {
        //digitalWrite(beeper, LOW);
      //}
    }
  }
  // if the file didn't open, print an error:
  else {
    //Serial.println("error opening test.txt");
  }


  //timer
  if (t0 >= 20) {
    if (i == 0) {
      t1 = millis();
    }
    i++;
    timer();
  }


  //delayMicroseconds(750); //850 teensy 3.6 256mhz 100hz 16475,1*mag,sd
  //delayMicroseconds(45); //45 teensy 4.0 600mhz 400hz 16475,2*mag,sd
  //delayMicroseconds(8000); //120 teensy 4.0 600mhz 350hz 16475,2*mag,sd   一樣code,但降幀,原400,後320,現調至350
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


void mag_data() {   //v2
  ID = mag_I2c;

  for (int j = 0; j < 1; j++) {
    tcaselect(mn[j]);
    writeRegister(mag_I2c, MMC5983MA_INTERNAL_CONTROL_0, 0x10); //get t,m data //only tm 0x02//set 0x10
    delayMicroseconds(3000);
    tm[0] = readRegister(MMC5983MA_TOUT);

    writeRegister(mag_I2c, MMC5983MA_INTERNAL_CONTROL_0, 0x01); //get t,m data
    delayMicroseconds(3000);
    readmultiRegister(MMC5983MA_XOUT_0, 7);

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
  int j = 1;
  writeRegister(mag_I2c, MMC5983MA_INTERNAL_CONTROL_0, 0x18); //get t,m data //only tm 0x02//set 0x10
  delayMicroseconds(3000);
  tm[0] = readRegister(MMC5983MA_TOUT);

  writeRegister(mag_I2c, MMC5983MA_INTERNAL_CONTROL_0, 0x01); //get t,m data
  delayMicroseconds(3000);
  readmultiRegister(MMC5983MA_XOUT_0, 7);

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
}


void tcaselect(uint8_t i) {
  if (i > 7) return;
  Wire.beginTransmission(TCAADDR);
  Wire.write(1 << i);
  Wire.endTransmission();
}


void timer() {
  timee = (millis() - t1) * 0.001;
  f = i / timee;
  if (f >= 500) { // fix ,999999
    f = 0;
  }

  if (i % 2000 == 0) { //設每幾筆資料儲存至SD卡 100約占0.03秒
    if (logFile.size() > maxsize) {
      logFileName = nextLogFile();
    }
    logFile.close(); // close the file
  }
}
