//proj_v1.3_v2 log
//upgrade mag code, led to beeper(but led), beept、 logFile.close to 1000,remove old rotor

#define TCAADDR 0x70

#include <Wire.h>
#include <SD.h>
#include <SPI.h>
#include"register.h"
// small board change:sd,led,beeper,hall,mpu6050 set&read tcaselect(2)
const int mag_I2c = 0x30; //
const int MPU_addr = 0x68;
const int acc_I2c = 0x1D; //0x53
const int SD_CS = 10; // teensy builtin
const int beeper = 4;
int ID, mn[6] = {2, 3, 4, 5, 6, 7}, delayy = 0, setF = 100;
long values[10], t[12];
double x[3], y[3], z[3], xm[7], ym[7], zm[7], gx1, gy1, gz1, t0, time = 0, f, tm[7], ta[3], tg; //, x1, y1, z1
String logFileName, accdata, gyrdata, magdata, logdata, mag2data, tmdata, halldata;// Rotordata;
File logFile;
unsigned long i = 0, t1, beepert;
const int H1 = 5, H2 = 6; //hall need wire
int lastR, Read, lastR2, Read2, maxsize = 209715200;//200mb=209715200(for excel),220mb=23068672,250mb = 250*2^20=262144000

void setup() {
  Serial.begin(115200);
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
    break;
  }

  logFileName = nextLogFile();

  //MMC5883MA n setting
  for (int j = 0; j < 2; j++) {
    tcaselect(mn[j]);
    //writeRegister(mag_I2c, MMC5883MA_INTERNAL_CONTROL_0, 0x08); //reseting
    //writeRegister(mag_I2c, MMC5883MA_INTERNAL_CONTROL_0, 0x16); //reseting
    //delay(30);
    //writeRegister(mag_I2c, MMC5883MA_INTERNAL_CONTROL_0, 0x04); //??
    //delay(30);
    writeRegister(mag_I2c , MMC5883MA_INTERNAL_CONTROL_1, 0x03); //2 200hz、350hz //3 600hz
    delay(30);
    writeRegister(mag_I2c , MMC5883MA_INTERNAL_CONTROL_0, 0x01); //initiate measurement
    delay(30);
  }

  //wakes up the MPU-6050
  tcaselect(2);
  writeRegister(MPU_addr, 0x6B, 0x80);// reset
  delay(100);
  writeRegister(MPU_addr, 0x6B, 0x00);// set to zero (wakes up the MPU-6050)
  delay(30);
  writeRegister(MPU_addr, 0x1B, 0x18);// set gyro range to +/-2000 degree
  delay(30);

  //adxl355 setting
  tcaselect(1);
  writeRegister(acc_I2c, RESET, 0x52);  // reset sensor
  delay(100);
  writeRegister(acc_I2c, POWER_CTL, 0x00);  // writing 0 to to enable sensor
  delay(30);
  writeRegister(acc_I2c, RANGE, 0x03);      // 01=2g 02=4g 03=8g  //LSB 2G:256000 8G:64000
  delay(30);
  writeRegister(acc_I2c, SELF_TEST, 0x00);  // writing 3 to to enable self test
  delay(30);

  //adxl355 2 setting
  tcaselect(0);
  writeRegister(acc_I2c, RESET, 0x52);  // reset sensor
  delay(100);
  writeRegister(acc_I2c, POWER_CTL, 0x00);  // writing 0 to to enable sensor
  delay(30);
  writeRegister(acc_I2c, RANGE, 0x03);      // 01=2g 02=4g 03=8g  //LSB 2G:256000 8G:64000
  delay(30);
  writeRegister(acc_I2c, SELF_TEST, 0x00);  // writing 3 to to enable self test
  delay(30);

  Serial.println(F("done initialize"));

  logFile = SD.open(logFileName, FILE_WRITE);
  if (logFile) {
    Serial.println(F("writing"));
  }
  // if the file didn't open, print an error:
  else {
    Serial.println(F("error opening file"));
    //return;
  }
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

  acc_data();
  mag_data();
  imu6050_data();

  /*Read = digitalRead(rinB) << 1 | digitalRead(rinA); // read encoder
    Read2 = digitalRead(rinB2) << 1 | digitalRead(rinA2); // read encoder2
    covert();

    Rotordata = String(Rotor) + ", " + String(Rotor2);*/

  halldata = String(analogRead(H1)) + ", " + String(analogRead(H2));

  accdata = String(x[1], 5) + ", " + String(y[1], 5) + ", " + String(z[1], 5) + ", " + String(x[2], 5) + ", " + String(y[2], 5) + ", " + String(z[2], 5);
  gyrdata = String(gx1, 5) + ", " + String(gy1, 5) + ", " + String(gz1, 5);
  magdata = String(xm[1], 5) + ", " + String(ym[1], 5) + ", " + String(zm[1], 5) + ", " + String(xm[2], 5) + ", " + String(ym[2], 5) + ", " + String(zm[2], 5) + ", " + String(xm[3], 5) + ", " + String(ym[3], 5) + ", " + String(zm[3], 5);
  //mag2data = String(xm[4], 5) + ", " + String(ym[4], 5) + ", " + String(zm[4], 5) + ", " + String(xm[5], 5) + ", " + String(ym[5], 5) + ", " + String(zm[5], 5) + ", " + String(xm[6], 5) + ", " + String(ym[6], 5) + ", " + String(zm[6], 5);
  //  tmdata = String(tm[1], 2) + ", " + String(tm[2], 2) + ", " + String(tm[3], 2) + ", " + String(tm[4], 2) + ", " + String(tm[5], 2) + ", " + String(tm[6], 2) + ", " + String(ta[1], 2) + ", " + String(ta[2], 2) + ", " + String(tg, 2);
  tmdata = String(tm[1], 2) + ", " + String(tm[2], 2) + ", " + String(tm[3], 2) + ", " + String(ta[1], 2) + ", " + String(ta[2], 2) + ", " + String(tg, 2);
  logdata = String(time, 3) + ", " + accdata + ", " + gyrdata + ", " + magdata + ", " + halldata + ", " + tmdata + ", " +  String(f, 2);// + ", " +  String(delayy);
  //Serial.println(logdata);
  //Serial.print(magdata);
  //Serial.print(",");
  //Serial.println(mag2data);
  Serial.println(gyrdata);


  if (!logFile) {
    logFile = SD.open(logFileName, FILE_WRITE);
  }
  if (logFile) {
    logFile.println(logdata);

    if (t0 >= 10) {
      if (i % 1000 == 0) { //v2
        digitalWrite(beeper, HIGH);
        beepert = i;
      }
      if (beepert + 10 == i) {
        digitalWrite(beeper, LOW);
      }
    }
  }
  else {
    Serial.println(F("error opening test.txt"));
  }

  //timer
  if (t0 >= 10) {
    if (i == 0) {
      t1 = millis();
    }
    i++;
    timer();
  }
  delayMicroseconds(70); //600Mhz proj_v1.3 中油 8吋管測
  //delayMicroseconds(3750); //5500 256hz 1mag
  //delayMicroseconds(5300); //5300 600Mhz proj_v1.3 100hz new sd//6400 600Mhz proj_v1.3//4400 256Mhz 2mag//5400 256Mhz 1mag //360 6mag 100hz
  //delayMicroseconds(0); //600Mhz proj_v1.3 中油
  //delayMicroseconds(delayy);
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

void imu6050_data() {
  ID = MPU_addr;

  tcaselect(2);

  readmultiRegister(0x41, 8);

  t[10] = values[0] << 8 | values[1];
  gx1 = values[2] << 8 | values[3];
  gy1 = values[4] << 8 | values[5];
  gz1 = values[6] << 8 | values[7];

  if (gx1 >= 0x8000)
    gx1 = gx1 -  0x10000;
  if (gy1 >= 0x8000)
    gy1 = gy1 -  0x10000;
  if (gz1 >= 0x8000)
    gz1 = gz1 -  0x10000;
    
  /*gx1 = gx1 / 131;  // +/-250
  gy1 = gy1 / 131;
  gz1 = gz1 / 131;*/
  gx1 = gx1 / 16.4;   // +/2000
  gy1 = gy1 / 16.4;
  gz1 = gz1 / 16.4;

  tg = (t[10] / 340) - 160.87 + 3.63;
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

    //x[0] = x[0] / 256000;  //2G
    //y[0] = y[0] / 256000;
    //z[0] = z[0] / 256000;
    x[0] = x[0] / 64000;  //8G
    y[0] = y[0] / 64000;
    z[0] = z[0] / 64000;

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

void tcaselect(uint8_t i) {
  if (i > 7) return;
  Wire.beginTransmission(TCAADDR);
  Wire.write(1 << i);
  Wire.endTransmission();
}

void timer() {
  time = (millis() - t1) * 0.001;
  f = i / time;
  if (f >= 500) { // fix ,999999
    f = 0;
  }

  if (i % 1000 == 0) { //v2
    if (logFile.size() > maxsize) {
      logFileName = nextLogFile();
    }
    logFile.close(); // close the file
  }
  /*if (time < 10) {
    if (f < setF && delayy != 0) {
    delayy -= 200;
    }
    if (f > setF) {
    delayy += 200;
    }
    }*/

  /*if (i % 500 == 0) { // beeper keep beep
    digitalWrite(beeper, HIGH);
    beepert = i;
    }
    if (beepert + 10 == i) { // beeper
    digitalWrite(beeper, LOW);
    }*/
}

/*void covert() {
  Read = graytobin(Read, 2);
  if ((Read > lastR) || (Read == 0 && lastR == 3)) {
    if (Read == 3 && lastR == 0) {
    }
    else
      Rotor -= 11.25;
  }
  if ((Read < lastR) || (Read == 3 && lastR == 0)) {
    if (Read == 0 && lastR == 3) {
    }
    else
      Rotor += 11.25;
  }
  //if (abs(Rotor) == 360)
  //Rotor = 0;
  lastR = Read;

  Read2 = graytobin(Read2, 2);
  if ((Read2 > lastR2) || (Read2 == 0 && lastR2 == 3)) {
    if (Read2 == 3 && lastR2 == 0) {
    }
    else
      Rotor2 -= 11.25;
  }
  if ((Read2 < lastR2) || (Read2 == 3 && lastR2 == 0)) {
    if (Read2 == 0 && lastR2 == 3) {
    }
    else
      Rotor2 += 11.25;
  }
  //if (abs(Rotor2) == 360)
  //Rotor2 = 0;
  lastR2 = Read2;
}*/

int graytobin(int grayVal, int nbits ) {
  int binVal = 0;
  bitWrite(binVal, nbits - 1, bitRead(grayVal, nbits - 1)); // MSB stays the same
  for (int b = nbits - 1; b > 0; b-- ) {
    // XOR bits
    if (bitRead(binVal, b) == bitRead(grayVal, b - 1)) { // binary bit and gray bit-1 the same
      bitWrite(binVal, b - 1, 0);
    }
    else {
      bitWrite(binVal, b - 1, 1);
    }
  }
  return binVal;
}
