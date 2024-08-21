#include <Wire.h>
#include <SD.h>
#include <SPI.h>
#include "DigitLedDisplay.h"

const int v_I2c = 0x40; //
const int MPU_addr = 0x68;
const int SD_CS = 10; // teensy builtin
int ID;
long values[20], t[12];
double x[3], y[3], z[3], xm[3], ym[3], zm[3], ttime = 0, f, ta[3];
String vdata, accdata, logdata;
unsigned long i = 0, t1, beepert;
int maxsize = 209715200;//209715200=200mb 262144000=250mb = 250*2^20;
File logFile;
String fileName;
String logFileName;
const int aCsw = 2;
const int bCsw = 3;
const int sdled = 4;
const int aled = 5;
const int bled = 6;
double aC[3], bC[3], I[3], timee;
unsigned long currentmillis = 0;
unsigned long headmillis = 0;
long initializetime = 500 * 1000;
byte start = 0, a, b;
DigitLedDisplay ld = DigitLedDisplay(9, 8, 7);
byte flag = 1;
int lognn;

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

void wireV()
{
  ID = v_I2c;

  readmultiRegister(0x02, 2);

  int16_t xmm = values[0] << 8 | values[1];

  xm[0] = xmm;
  //xm[1] = xm[0]/2048;
  xm[1] = (xmm >> 3) * 4 * 0.001;
  xm[2] = xm[1] * 9.023;

  readmultiRegister(0x01, 2);

  int16_t ymm = values[0] << 8 | values[1];

  ym[0] = ymm;
  //ym[1] = ym[0]/2048;
  ym[1] = ym[0] * 0.01;
  ym[2] = ym[1] * 9.023;

  //readmultiRegister(0x05, 2);

  //int16_t zmm = values[0] << 8 | values[1];

  zm[0] = ym[1] / 0.1;
}


void save() {
  if (!logFile) {
    logFile = SD.open(logFileName, FILE_WRITE);
  }
  if (logFile) {
    //digitalWrite(led, HIGH);
    logFile.print(vdata);
    logFile.print(", ");
    logFile.print(accdata);
    digitalWrite(sdled, HIGH);
  }
  else {
    Serial.println(F("error opening test.txt"));
    digitalWrite(sdled, LOW);
  }
  logFile.close();
}

void imu6050_data() {
  ID = MPU_addr;

  readmultiRegister(0x3B, 8);

  x[0] = values[0] << 8 | values[1];
  y[0] = values[2] << 8 | values[3];
  z[0] = values[4] << 8 | values[5];
  t[10] = values[6] << 8 | values[7];

  if (x[0] >= 0x8000) {
    x[0] = x[0] -  0x10000;
  }
  if (y[0] >= 0x8000) {
    y[0] = y[0] -  0x10000;
  }
  if (z[0] >= 0x8000) {
    z[0] = z[0] -  0x10000;
  }

  x[0] = x[0] / 0x4000;
  y[0] = y[0] / 0x4000;
  z[0] = z[0] / 0x4000;

  ta[0] = (t[10] / 340) - 160.87 + 3.63;
}

String nextLogFile(void) {
  String filename;
  int logn = 0;
  lognn = 0;
  for (int i = 0; i < 999; i++) {
    // Construct a file with PREFIX[Index].SUFFIX
    filename = String("M_log") + String(logn) + String(".csv");
    // If the file name doesn't exist, return it
    if (!SD.exists(filename))
    {
      return filename;
    }
    // Otherwise increment the index, and try again
    logn++;
    lognn++;
  }
  return "";
}


void setup()
{
  Serial.begin(115200);
  Wire.begin();
  Wire.setClock(100000);

  ld.setBright(15);
  ld.setDigitLimit(8);

  pinMode(SD_CS, OUTPUT);
  pinMode(aCsw, OUTPUT);
  pinMode(bCsw, OUTPUT);
  pinMode(aled, OUTPUT);
  pinMode(bled, OUTPUT);
  pinMode(sdled, OUTPUT);
  digitalWrite(sdled, HIGH);

  /*if (!SD.begin(SD_CS)) {
    Serial.println(F("Card failed, or not present"));
    digitalWrite(sdled, LOW);
    }

    logFileName = nextLogFile();*/

  //wakes up the MPU-6050
  writeRegister(MPU_addr, 0x6B, 0x80);// reset
  delay(100);
  writeRegister(MPU_addr, 0x6B, 0x00);// set to zero (wakes up the MPU-6050)
  delay(30);

  /*logFile = SD.open(logFileName, FILE_WRITE);
    if (logFile) {
    Serial.println(F("writing"));
    logFile.print(" ");
    digitalWrite(sdled, HIGH);
    ld.write(7, B00001110);
    ld.write(6, B01111110);
    ld.write(5, B01011110);
    ld.printDigit(lognn, 0);
    }
    // if the file didn't open, print an error:
    else {
    Serial.println(F("error opening file"));
    digitalWrite(sdled, LOW);
    }
    logFile.close();*/

  ld.write(7, B00001110);
  ld.write(6, B01111110);
  ld.write(5, B01011110);

  Serial.println(F("done initialize"));
  headmillis = micros();
}


void loop() {
  while (1) { //test
    //Serial.print(F("A loop "));
    digitalWrite(aled, HIGH);
    digitalWrite(bled, LOW);
    digitalWrite(aCsw, LOW);
    digitalWrite(bCsw, LOW);
    digitalWrite(aCsw, HIGH);
    delay(10);
    wireV();
    aC[0] = xm[2];
    digitalWrite(aCsw, LOW);
    imu6050_data();
    currentmillis = micros() - headmillis;
    timee = currentmillis * 0.000001;
    //vdata = "A, " + String(timee, 3) + ", " + String(xm[2], 5) + ", " + String(ym[2], 5) + ", " + String(xm[1], 5) + ", " + String(ym[1], 5) + ", " + String(zm[0], 5);
    vdata = "A, " + String(timee, 3) + ", " + String(xm[2], 5);
    accdata = String(x[0], 5) + ", " + String(y[0], 5) + ", " + String(z[0], 5) + ", " + String(ta[0], 2);
    //Serial.println(vdata);
    //Serial.println(accdata);
    //Serial.print(String(xm[2], 5));
    //save();
    cleara();
    double aa = int(xm[2]);
    int bb = round(xm[2] * 10);
    bb = bb % 10;
    aa = abs(aa);
    bb = abs(bb);
    printDigit(aa, 5);
    printDigit(bb, 4);
    Serial.println(aC[0], 5);
    delay(1000);

    //Serial.print(F("B loop "));
    digitalWrite(aled, LOW);
    digitalWrite(bled, HIGH);
    digitalWrite(aCsw, LOW);
    digitalWrite(bCsw, LOW);
    digitalWrite(bCsw, HIGH);
    delay(10);
    wireV();
    bC[0] = xm[2];
    digitalWrite(bCsw, LOW);
    imu6050_data();
    currentmillis = micros() - headmillis;
    timee = currentmillis * 0.000001;
    //vdata = "B, " + String(timee, 3) + ", " + String(xm[2], 5) + ", " + String(ym[2], 5) + ", " + String(xm[1], 5) + ", " + String(ym[1], 5) + ", " + String(zm[0], 5);
    vdata = "B, " + String(timee, 3) + ", " + String(xm[2], 5);
    accdata = String(x[0], 5) + ", " + String(y[0], 5) + ", " + String(z[0], 5) + ", " + String(ta[0], 2) + "\n";
    //Serial.println(vdata);
    //Serial.println(accdata);
    //Serial.println(String(xm[2], 5));
    //save();
    clearb();
    aa = int(xm[2]);
    bb = round(xm[2] * 10);
    bb = bb % 10;
    aa = abs(aa);
    bb = abs(bb);
    printDigit(aa, 1);
    printDigit(bb, 0);
    Serial.println(bC[0], 5);
    delay(1000);
  }




  char i;
  if (Serial.available() > 0) {
    // digitalWrite(sdled, HIGH);
    if (!logFile) {
      logFile = SD.open(logFileName, FILE_WRITE);
    }
    i = Serial.read();
    if (i == 'A') {
      digitalWrite(aled, HIGH);
      digitalWrite(bled, LOW);
      digitalWrite(aCsw, LOW);
      digitalWrite(bCsw, LOW);
      digitalWrite(aCsw, HIGH);
      delay(10);
      wireV();
      digitalWrite(aCsw, LOW);
      imu6050_data();
      currentmillis = micros() - headmillis;
      timee = currentmillis * 0.000001;
      //vdata = "A, " + String(timee, 3) + ", " + String(xm[2], 5) + ", " + String(ym[2], 5) + ", " + String(xm[1], 5) + ", " + String(ym[1], 5) + ", " + String(zm[0], 5);
      vdata = "A, " + String(timee, 3) + ", " + String(xm[2], 5);
      accdata = String(x[0], 5) + ", " + String(y[0], 5) + ", " + String(z[0], 5) + ", " + String(ta[0], 2) + ", ";
      Serial.print(vdata);
      Serial.print(", ");
      Serial.print(accdata);
      //Serial.println(String(xm[2], 5));
      //save();
      cleara();
      double aa = int(xm[2]);
      int bb = round(xm[2] * 10);
      bb = bb % 10;
      aa = abs(aa);
      bb = abs(bb);
      printDigit(aa, 5);
      printDigit(bb, 4);
    }

    if (i == 'B') {
      digitalWrite(bled, HIGH);
      digitalWrite(aled, LOW);
      digitalWrite(aCsw, LOW);
      digitalWrite(bCsw, LOW);
      digitalWrite(bCsw, HIGH);
      delay(10);
      wireV();
      digitalWrite(bCsw, LOW);
      imu6050_data();
      currentmillis = micros() - headmillis;
      timee = currentmillis * 0.000001;
      //vdata = "B, " + String(timee, 3) + ", " + String(xm[2], 5) + ", " + String(ym[2], 5) + ", " + String(xm[1], 5) + ", " + String(ym[1], 5) + ", " + String(zm[0], 5);
      vdata = "B, " + String(timee, 3) + ", " + String(xm[2], 5);
      accdata = String(x[0], 5) + ", " + String(y[0], 5) + ", " + String(z[0], 5) + ", " + String(ta[0], 2) + "\n";
      Serial.print(vdata);
      Serial.print(", ");
      Serial.print(accdata);
      //Serial.println(String(xm[2], 5));
      //save();
      clearb();
      double aa = int(xm[2]);
      int bb = round(xm[2] * 10);
      bb = bb % 10;
      aa = abs(aa);
      bb = abs(bb);
      printDigit(aa, 1);
      printDigit(bb, 0);
    }
  }
}



void printDigit(long number, byte startDigit) {
  String figure = String(number);
  int figureLength = figure.length();

  int parseInt;
  char str[2];
  for (int i = 0; i < figure.length(); i++) {
    str[0] = figure[i];
    str[1] = '\0';
    parseInt = (int) strtol(str, NULL, 10);

    table(figureLength - i + startDigit, parseInt);
  }
}
void table(byte address, int val) {//address = 1~8(位置) val = 數值
  byte tableValue;
  tableValue = pgm_read_byte_near(charTable + val);

  if (address == 2 || address == 6) {
    tableValue = tableValue + 128; //加上小數點
  }
  write(address, tableValue);
}

void write(volatile byte address, volatile byte data) {
  digitalWrite(8, LOW);
  shiftOut(9, 7, MSBFIRST, address);
  shiftOut(9, 7, MSBFIRST, data);
  digitalWrite(8, HIGH);
}

void cleara() {
  for (int i = 5; i <= 8; i++) {
    write(i, B00000000);
  }
}
void clearb() {
  for (int i = 1; i <= 4; i++) {
    write(i, B00000000);
  }
}
