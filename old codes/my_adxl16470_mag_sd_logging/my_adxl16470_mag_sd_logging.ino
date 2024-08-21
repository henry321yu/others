/*log 
 * 7/7 add hall
 * 7/9 發現頻率又降了?(319hz)
 * 7/12 換新SD卡 升至(339hz)
*/

#include <SPI.h>
#include <ADIS16470.h>
#include <SD.h>
#include <Wire.h>
#define TCAADDR 0x70
#include"register.h"

const int SD_CS = 10;
const int acc16475_CS = 15;
const int acc16475_DR = 2;
const int acc16475_RSET = 3;
int16_t regData[32];
byte accdataN = 24;
int32_t gxo, gyo, gzo, axo, ayo, azo = 0;
int32_t gxb, gyb, gzb, axb, ayb, azb = 0;
double ax, ay, az, gx, gy, gz;
byte regread0 , regread1, regread2, regread3;
String logFileName, accdata, gyrdata, magdata, logdata, mag2data, tmdata, halldata;// Rotordata;
File logFile;
const int beeper = 4;


const int mag_I2c = 0x30; //
int ID, mn[6] = {2, 3, 4, 5, 6, 7}, delayy = 0, setF = 100;
long values[10], t[12];
double x[3], y[3], z[3], xm[7], ym[7], zm[7], gx1, gy1, gz1, t0, timee = 0, f, tm[7], tg; //, x1, y1, z1
int maxsize = 262144000;//250mb = 250*2^20;
unsigned long i = 0, t1, beepert;
const int H1 = A9, H2 = A8; //hall need wire



ADIS16470 IMU(acc16475_CS, acc16475_DR, acc16475_RSET); // Chip Select, Data Ready, Reset Pin Assignments

void setup() {
  Serial.begin(115200); // Initialize serial output via USB
  //IMU.configSPI(); // Configure SPI communication
  Wire.begin();
  Wire.setClock(400000);

  pinMode(H1, INPUT);
  pinMode(H2, INPUT);
  pinMode(beeper, OUTPUT);

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

  //MMC5883MA n setting
  //for (int j = 0; j < 2; j++) {
    tcaselect(mn[j]);
    writeRegister(mag_I2c , MMC5883MA_INTERNAL_CONTROL_1, 0x03); //2 200hz、350hz //3 600hz
    delay(30);
    writeRegister(mag_I2c , MMC5883MA_INTERNAL_CONTROL_0, 0x01); //initiate measurement
    delay(30);
  //}


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

  logFileName = nextLogFile();
  Serial.println(F("done initialize"));

  delay(500);
  logFile = SD.open(logFileName, FILE_WRITE);
  if (logFile) {
    Serial.println(F("writing"));
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

  halldata = String(analogRead(H1)) + ", " + String(analogRead(H2));
  //halldata = String(analogRead(H2));
  accdata = String(ax, 7) + ", " + String(ay, 7) + ", " + String(az, 7);
  gyrdata = String(gx, 6) + ", " + String(gy, 6) + ", " + String(gz, 6);

  mag_data();
  magdata = String(xm[1], 5) + ", " + String(ym[1], 5) + ", " + String(zm[1], 5) + ", " + String(xm[2], 5) + ", " + String(ym[2], 5) + ", " + String(zm[2], 5);// + ", " + String(xm[3], 5) + ", " + String(ym[3], 5) + ", " + String(zm[3], 5);
  tmdata = String(tm[1], 2) + ", " + String(tm[2], 2);

  logdata = String(timee, 3) + ", " + accdata + "," + gyrdata + ", " + magdata + ", " + halldata + ", " + tmdata + ", " + String(f, 2);
  Serial.println(logdata);
  //Serial.println( halldata);

  if (!logFile) {
    logFile = SD.open(logFileName, FILE_WRITE);
  }
  if (logFile) {
    logFile.println(logdata);

    if (t0 >= 20) {
      if (i % 2000 == 0) { //v2
        digitalWrite(beeper, HIGH);
        beepert = i;
      }
      if (beepert + 10 == i) {
        digitalWrite(beeper, LOW);
      }
    }
  }
  // if the file didn't open, print an error:
  else {
    Serial.println("error opening test.txt");
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

String nextLogFile(void) {
  String filename;
  int logn = 0;
  for (int i = 0; i < 999; i++) {
    // Construct a file with PREFIX[Index].SUFFIX
    filename = String("log") + String(logn) + String(".csv");
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

  for (int j = 0; j < 2; j++) {
    tcaselect(mn[j]);

    writeRegister(mag_I2c, MMC5883MA_INTERNAL_CONTROL_0, 0x02); //get t,m data
    delayMicroseconds(30);
    tm[0] = readRegister(MMC5883MA_TEMPERATURE);

    writeRegister(mag_I2c, MMC5883MA_INTERNAL_CONTROL_0, 0x01); //get t,m data
    delayMicroseconds(30);
    readmultiRegister(MMC5883MA_XOUT_LOW, 6);
    t[3] = values[1] << 8 | values[0];
    t[4] = values[3] << 8 | values[2];
    t[5] = values[5] << 8 | values[4];

    xm[0] = t[3];
    ym[0] = t[4];
    zm[0] = t[5];

    xm[0] = (xm[0] - 0x8000) / 40.96;
    ym[0] = (ym[0] - 0x8000) / 40.96;
    zm[0] = (zm[0] - 0x8000) / 40.96;

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
      case 2:
        tm[j + 1] = tm[0] - 10.1; break;
      case 3:
        tm[j + 1] = tm[0] - 15.7; break;
      case 4:
        tm[j + 1] = tm[0] - 17.8; break;
      case 5:
        tm[j + 1] = tm[0] - 15; break;
    }
  }
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
