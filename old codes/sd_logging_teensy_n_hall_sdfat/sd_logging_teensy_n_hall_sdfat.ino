#include "SdFat.h"
#include "sdios.h"
#include "FreeStack.h"

// SD_FAT_TYPE = 0 for SdFat/File as defined in SdFatConfig.h,
// 1 for FAT16/FAT32, 2 for exFAT, 3 for FAT16/FAT32 and exFAT.
#define SD_FAT_TYPE 3
/*
  Change the value of SD_CS_PIN if you are using SPI and
  your hardware does not use the default value, SS.
  Common values are:
  Arduino Ethernet shield: pin 4
  Sparkfun SD shield: pin 8
  Adafruit SD shields and modules: pin 10
*/
// SDCARD_SS_PIN is defined for the built-in SD on some boards.
#ifndef SDCARD_SS_PIN
const uint8_t SD_CS_PIN = SS;
#else  // SDCARD_SS_PIN
// Assume built-in SD is used.
const uint8_t SD_CS_PIN = SDCARD_SS_PIN;
#endif  // SDCARD_SS_PIN

// Try max SPI clock for an SD. Reduce SPI_CLOCK if errors occur.
#define SPI_CLOCK SD_SCK_MHZ(50)

// Try to select the best SD card configuration.
#if HAS_SDIO_CLASS
#define SD_CONFIG SdioConfig(FIFO_SDIO)
#elif ENABLE_DEDICATED_SPI
#define SD_CONFIG SdSpiConfig(SD_CS_PIN, DEDICATED_SPI, SPI_CLOCK)
#else  // HAS_SDIO_CLASS
#define SD_CONFIG SdSpiConfig(SD_CS_PIN, SHARED_SPI, SPI_CLOCK)
#endif  // HAS_SDIO_CLASS


#if SD_FAT_TYPE == 0
SdFat sd;
File file;
#elif SD_FAT_TYPE == 1
SdFat32 sd;
File32 file;
#elif SD_FAT_TYPE == 2
SdExFat sd;
ExFile file;
#elif SD_FAT_TYPE == 3
SdFs sd;
FsFile file;
#else  // SD_FAT_TYPE
#error Invalid SD_FAT_TYPE
#endif  // SD_FAT_TYPE

// Serial output stream
ArduinoOutStream cout(Serial);
//------------------------------------------------------------------------------
// Store error strings in flash to save RAM.
#define error(s) sd.errorHalt(&Serial, F(s))
//------------------------------------------------------------------------------ sd setting

#define TCAADDR 0x70

#include <Wire.h>
#include <SPI.h>
#include"register.h"

const int mag_I2c = 0x30; //
const int MPU_addr = 0x68;
const int acc_I2c = 0x1D; //0x53
const int led = 13; //
const int beeper = 35;
int ID, mn[6] = {2, 4, 4, 5, 6, 7}, delayy = 0, setF = 100;
long values[10], t[12];
double x[3], y[3], z[3], xm[7], ym[7], zm[7], gx1, gy1, gz1, t0, tt = 0, f, tm[7], ta[3], tg; //, x1, y1, z1
String logfileName = "", accdata, gyrdata, magdata, logdata, mag2data, tmdata, halldata; // Rotordata;
unsigned long i = 0, t1, beepert;
//const int rinA = 24, rinB = 25;
//const int rinA2 = 26, rinB2 = 27;
const int H1 = 2, H2 = 3;//H1 = A20, H2 = A19; //wired
int lastR, Read, lastR2, Read2, maxsize = 262144000;//250mb = 250*2^20;
double Rotor = 0, Rotor2 = 0;

void setup() {
  Serial.begin(115200);
  Wire.begin();
  Wire.setClock(400000);

  if (!sd.begin(SD_CONFIG)) {
    error("sd begin failed");
  }
  logfileName = nextLogFile();


  //MMC5883MA n setting
  for (int j = 0; j < 2; j++) {
    tcaselect(mn[j]);
    writeRegister(mag_I2c, MMC5883MA_INTERNAL_CONTROL_0, 0x08); //reseting
    delay(30);
    writeRegister(mag_I2c, MMC5883MA_INTERNAL_CONTROL_0, 0x04);
    delay(30);
    writeRegister(mag_I2c , MMC5883MA_INTERNAL_CONTROL_0, 0x01); //initiate measurement
    delay(30);
    writeRegister(mag_I2c , MMC5883MA_INTERNAL_CONTROL_1, 0x03); //3 600hz
    delay(30);
  }

  //wakes up the MPU-6050
  writeRegister(MPU_addr, 0x6B, 0x80);// reset
  delay(100);
  writeRegister(MPU_addr, 0x6B, 0x00);// set to zero (wakes up the MPU-6050)
  delay(30);

  //adxl355 setting
  tcaselect(1);
  writeRegister(acc_I2c, RESET, 0x52);  // reset sensor
  delay(100);
  writeRegister(acc_I2c, POWER_CTL, 0x00);  // writing 0 to to enable sensor
  delay(30);
  writeRegister(acc_I2c, RANGE, 0x02);
  delay(30);
  writeRegister(acc_I2c, SELF_TEST, 0x00);  // writing 3 to to enable self test
  delay(30);

  //adxl355 2 setting
  tcaselect(0);
  writeRegister(acc_I2c, RESET, 0x52);  // reset sensor
  delay(100);
  writeRegister(acc_I2c, POWER_CTL, 0x00);  // writing 0 to to enable sensor
  delay(30);
  writeRegister(acc_I2c, RANGE, 0x02);
  delay(30);
  writeRegister(acc_I2c, SELF_TEST, 0x00);  // writing 3 to to enable self test
  delay(30);

  Serial.println(F("done initialize"));


  // open or create file - truncate existing file.
  file = sd.open(logfileName, FILE_WRITE);
  if (file) {
    cout << F("writing\n");
  }
  else {
    cout << F("error opening file");
  }
  pinMode(led, OUTPUT);
  pinMode(beeper, OUTPUT);
  digitalWrite(led, HIGH);
  delay(2000);
  digitalWrite(led, LOW);
  delay(1000);
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
  tmdata = String(tm[1], 2) + ", " + String(tm[2], 2) + ", " + String(tm[3], 2) + ", " + String(ta[1], 2) + ", " + String(ta[2], 2) + ", " + String(tg, 2);
  //tmdata = String(tm[1], 2) + ", " + String(tm[2], 2) + ", " + String(tm[3], 2) + ", " + String(tm[4], 2) + ", " + String(tm[5], 2) + ", " + String(tm[6], 2) + ", " + String(ta[1], 2) + ", " + String(ta[2], 2) + ", " + String(tg, 2);
  logdata = String(tt, 3) + ", " + accdata + ", " + gyrdata + ", " + magdata + ", " + halldata + ", " + tmdata + ", " +  String(f, 2); // + ", " +  String(delayy);
  Serial.println(logdata);
  //Serial.print(magdata);


  if (! file) {
    file = sd.open(logfileName, FILE_WRITE);
  }
  if ( file) {
    digitalWrite(led, HIGH);
    file.println(logdata);

    if (i % 2000 == 0 && t0 > 10) {
      digitalWrite(beeper, HIGH);
      beepert = i;
    }
    if (beepert + 40 == i) {
      digitalWrite(beeper, LOW);
    }
  }
  else {
    Serial.println(F("error opening"));
  }

  //timer
  if (t0 >= 10) {
    if (i == 0) {
      t1 = millis();
    }
    i++;
    timer();
  }
  delayMicroseconds(205); //205(150(all data)) teensy 4.1 400hz(all data) //7500 teensy 4.1 100hz(remember change M set,beeper,timer)
  //delayMicroseconds(3750); //5500 256hz 1mag
  //delayMicroseconds(4400); //4400 256Mhz 2mag//5400 256Mhz 1mag //360 6mag 100hz
  //delayMicroseconds(delayy);
  digitalWrite(led, LOW);
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

void mag_data() {
  ID = mag_I2c;

  for (int j = 0; j < 1; j++) {
    tcaselect(mn[j]);
    writeRegister(mag_I2c, MMC5883MA_INTERNAL_CONTROL_0, 0x02); //temperature reading
    //delayMicroseconds(180);
    tm[0] = readRegister(MMC5883MA_TEMPERATURE);

    writeRegister(mag_I2c, MMC5883MA_INTERNAL_CONTROL_0, 0x09); //set
    //delayMicroseconds(200);

    readmultiRegister(MMC5883MA_XOUT_LOW, 6); // read sensor
    t[0] = values[1] << 8 | values[0];
    t[1] = values[3] << 8 | values[2];
    t[2] = values[5] << 8 | values[4];

    writeRegister(mag_I2c, MMC5883MA_INTERNAL_CONTROL_0, 0x05); //reset
    //delayMicroseconds(200);
    readmultiRegister(MMC5883MA_XOUT_LOW, 6);

    t[3] = values[1] << 8 | values[0];
    t[4] = values[3] << 8 | values[2];
    t[5] = values[5] << 8 | values[4];

    xm[0] = (t[0] + t[3]) / 2;
    ym[0] = (t[1] + t[4]) / 2;
    zm[0] = (t[2] + t[5]) / 2;

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
  gx1 = gx1 / 131;
  gy1 = gy1 / 131;
  gz1 = gz1 / 131;

  tg = (t[10] / 340) - 160.87 + 3.63;
}

void acc_data() {
  ID = acc_I2c;

  for (int j = 0; j < 2; j++) {
    tcaselect(j);

    readmultiRegister(XDATA3, 9);

    x[0] = values[0] << 12 | values[1] << 4 | values[2] >> 4;
    y[0] = values[3] << 12 | values[4] << 4 | values[5] >> 4;
    z[0] = values[6] << 12 | values[7] << 4 | values[8] >> 4;

    if (x[0] >= 0x80000)
      x[0] = x[0] - (2 * 0x80000);
    if (y[0] >= 0x80000)
      y[0] = y[0] - (2 * 0x80000);
    if (z[0] >= 0x80000)
      z[0] = z[0] - (2 * 0x80000);

    t[8] = readRegister(TEMP2);
    t[9] = readRegister(TEMP1);

    ta[0] = (t[8] << 8) + t[9];
    ta[0] = ((1852 - ta[0]) / 9.05);

    //x = x / 256000; //2G
    //y = y / 256000;
    //z = z / 256000;
    x[0] = x[0] / 128000;  //4G
    y[0] = y[0] / 128000;
    z[0] = z[0] / 128000;

    if (j == 0) { //switching data to acc1,acc2
      x[2] = x[0];
      y[2] = y[0];
      z[2] = z[0];
      ta[2] = ta[0] + 28.52;
    }
    else {
      x[1] = x[0];
      y[1] = y[0];
      z[1] = z[0];
      ta[1] = ta[0] + 27.2;
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
    if (!sd.exists(filename))
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
  tt = (millis() - t1) * 0.001;
  f = i / tt;
  if (f >= 500) { // fix ,999999
    f = 0;
  }

  if (i % 1200 == 0) {
    if ( file.size() > maxsize)
    {
      logfileName = nextLogFile();
    }
    file.close(); // close the file
    if (i % 1000 == 0) {
      //Serial.print(F("freqency= ")); Serial.print(f); Serial.println(F(" Hz"));
    }
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

void covert() {
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
}

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
