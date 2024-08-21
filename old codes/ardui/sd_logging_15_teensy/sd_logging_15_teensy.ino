#define TCAADDR 0x70

#include <Wire.h>
#include <SD.h>
#include <SPI.h>
#include"register.h"

const int mag_I2c = 0x30; //
const int MPU_addr = 0x68;
const int acc_I2c = 0x1D; //0x53
const int SD_CS = BUILTIN_SDCARD; // teensy builtin
const int led = 5; //
int ID;
long values[10], t[12];
double x[3], y[3], z[3],xm[3], ym[3], zm[3], gx1, gy1, gz1, time, f, tm[3], at[3]; //, x1, y1, z1
String logFileName, accdata, gyrdata, magdata, logdata, mag2data, tmdata, Rotordata, acc2data;
File logFile;
unsigned long i = 0, t0;
const int rinA = 2, rinB = 3;
const int rinA2 = 11, rinB2 = 12;
int lastR, Read, lastR2, Read2;
double Rotor = 0, Rotor2 = 0;


void setup() {
  Serial.begin(115200);
  Wire.begin();
  Wire.setClock(400000);

  // set encoder
  pinMode(rinA, INPUT);
  pinMode(rinB, INPUT);
  Read = digitalRead(rinB) << 1 | digitalRead(rinA);
  Read = graytobin(Read, 2);
  lastR = Read;


  // set encoder2
  pinMode(rinA2, INPUT);
  pinMode(rinB2, INPUT);
  Read2 = digitalRead(rinB2) << 1 | digitalRead(rinA2);
  Read2 = graytobin(Read2, 2);
  lastR2 = Read2;


  if (!SD.begin(SD_CS)) {
    Serial.println(F("Card failed, or not present"));
  }
  pinMode(SD_CS, OUTPUT);
  logFileName = nextLogFile();

  //MMC5883MA 1 setting
  tcaselect(0);
  writeRegister(mag_I2c, MMC5883MA_INTERNAL_CONTROL_0, 0x08); //reseting
  delay(30);
  writeRegister(mag_I2c, MMC5883MA_INTERNAL_CONTROL_0, 0x04);
  delay(30);
  writeRegister(mag_I2c , MMC5883MA_INTERNAL_CONTROL_0, 0x01); //initiate measurement
  delay(30);
  writeRegister(mag_I2c , MMC5883MA_INTERNAL_CONTROL_1, 0x02); //200hz
  delay(30);

  //MMC5883MA 2 setting
  tcaselect(3);
  writeRegister(mag_I2c, MMC5883MA_INTERNAL_CONTROL_0, 0x08);//reseting
  delay(30);
  writeRegister(mag_I2c, MMC5883MA_INTERNAL_CONTROL_0, 0x04);
  delay(30);
  writeRegister(mag_I2c , MMC5883MA_INTERNAL_CONTROL_0, 0x01); //initiate measurement
  delay(30);
  writeRegister(mag_I2c , MMC5883MA_INTERNAL_CONTROL_1, 0x02); //200hz
  delay(30);

  //wakes up the MPU-6050
  tcaselect(1);
  writeRegister(MPU_addr, 0x6B, 0x80);// reset
  delay(100);
  writeRegister(MPU_addr, 0x6B, 0x00);// set to zero (wakes up the MPU-6050)
  delay(30);

  //adxl355 setting
  tcaselect(2);
  writeRegister(acc_I2c, RESET, 0x52);  // reset sensor
  delay(100);
  writeRegister(acc_I2c, POWER_CTL, 0x00);  // writing 0 to to enable sensor
  delay(30);
  writeRegister(acc_I2c, RANGE, 0x02);
  delay(30);
  writeRegister(acc_I2c, SELF_TEST, 0x00);  // writing 3 to to enable self test
  delay(30);

  //adxl355 2 setting
  tcaselect(4);
  writeRegister(acc_I2c, RESET, 0x52);  // reset sensor
  delay(100);
  writeRegister(acc_I2c, POWER_CTL, 0x00);  // writing 0 to to enable sensor
  delay(30);
  writeRegister(acc_I2c, RANGE, 0x02);
  delay(30);
  writeRegister(acc_I2c, SELF_TEST, 0x00);  // writing 3 to to enable self test
  delay(30);

  Serial.println(F("done initialize"));


  logFile = SD.open(logFileName, FILE_WRITE);
  if (logFile) {
    Serial.println(F("writing"));
    //test0.print("當您取消產品保修時。然後你真正擁有了這個產品。");
    //test0.close(); // close the file
  }
  // if the file didn't open, print an error:
  else {
    Serial.println(F("error opening file"));
    //return;
  }
  pinMode(led, OUTPUT);
  pinMode(4, OUTPUT);
  digitalWrite(4, LOW);
  digitalWrite(led, HIGH);
  delay(2000);
  digitalWrite(led, LOW);
  delay(1000);

  t0 = millis();
}
void loop() {
  acc_data();
  mag_data();
  imu6050_data();
  mag2_data();
  //acc2_data();

  Read = digitalRead(rinB) << 1 | digitalRead(rinA); // read encoder
  Read2 = digitalRead(rinB2) << 1 | digitalRead(rinA2); // read encoder2
  covert();
  //covert2();
  //Serial.print ("Rotor : "); Serial.print(Rotor); Serial.print ("  ");
  //Serial.print ("Rotor2 : "); Serial.print(Rotor2); Serial.println ("  ");

  Rotordata = String(Rotor) + ", " + String(Rotor2);
  accdata = String(x[1], 5) + ", " + String(y[1], 5) + ", " + String(z[1], 5);
  //acc2data = String(x[2], 5) + ", " + String(y[2], 5) + ", " + String(z[2], 5);
  gyrdata = String(gx1, 5) + ", " + String(gy1, 5) + ", " + String(gz1, 5);
  magdata = String(xm[1], 5) + ", " + String(ym[1], 5) + ", " + String(zm[1], 5);
  mag2data = String(xm[2], 5) + ", " + String(ym[2], 5) + ", " + String(zm[2], 5);
  tmdata = String(tm[1], 2) + ", " + String(tm[2], 2) + ", " + String(at[1], 2);
  //tmdata = String(tm[1], 2) + ", " + String(tm[2], 2) + ", " + String(at[1], 2) + ", " + String(at[2], 2);
  logdata = String(time, 3) + ", " + accdata + ", " + gyrdata + ", " + magdata + ", " + mag2data + ", " + tmdata + ", " + Rotordata;
  //logdata = String(time, 3) + ", " + accdata + ", " + acc2data + ", " + gyrdata + ", " + magdata + ", " + mag2data + ", " + tmdata + ", " + Rotordata;
  Serial.println(logdata);

  //delay(1);
  if (!logFile) {
    logFile = SD.open(logFileName, FILE_WRITE);
    //Serial.println(F("file opened"));
  }
  //delay(1);
  if (logFile) {
    //Serial.println(F("writing"));
    digitalWrite(led, HIGH);
    logFile.println(logdata);

    //logFile.close(); // close the file
  }
  // if the file didn't open, print an error:
  else {
    Serial.println(F("error opening test.txt"));
    //return;
  }
  timer();
  //delayMicroseconds(40);//485 w'rotor
  //delayMicroseconds(200);// m12 tm
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
  if (t[7] == 3)
    tcaselect(3);
  else
    tcaselect(0);

  ID = mag_I2c;

  writeRegister(mag_I2c, MMC5883MA_INTERNAL_CONTROL_0, 0x02); //temperature reading
  delayMicroseconds(100);
  tm[0] = readRegister(MMC5883MA_TEMPERATURE);

  writeRegister(mag_I2c, MMC5883MA_INTERNAL_CONTROL_0, 0x09); //set
  delayMicroseconds(200);

  readmultiRegister(MMC5883MA_XOUT_LOW, 6); // read sensor
  t[0] = values[1] << 8 | values[0];
  t[1] = values[3] << 8 | values[2];
  t[2] = values[5] << 8 | values[4];

  writeRegister(mag_I2c, MMC5883MA_INTERNAL_CONTROL_0, 0x05); //reset
  delayMicroseconds(200);
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

  if (t[7] == 3) { //switching data to m1,m2
    xm[2] = xm[0];
    ym[2] = ym[0];
    zm[2] = zm[0];
    tm[2] = tm[0] - 17.8;
  }
  else {
    xm[1] = xm[0];
    ym[1] = ym[0];
    zm[1] = zm[0];
    tm[1] = tm[0] - 16.4;
  }
}
void mag2_data() {
  t[7] = 3;
  mag_data();
  t[7] = 0;
}

void imu6050_data() {
  tcaselect(1);
  ID = MPU_addr;
  readmultiRegister(0x43, 6);

  gx1 = values[0] << 8 | values[1];
  gy1 = values[2] << 8 | values[3];
  gz1 = values[4] << 8 | values[5];

  if (gx1 >= 0x8000)
    gx1 = gx1 -  0x10000;
  if (gy1 >= 0x8000)
    gy1 = gy1 -  0x10000;
  if (gz1 >= 0x8000)
    gz1 = gz1 -  0x10000;
  gx1 = gx1 / 131;
  gy1 = gy1 / 131;
  gz1 = gz1 / 131;
}

void acc_data() {
  if (t[10] == 5)
    tcaselect(4);
  else
    tcaselect(2);

  ID = acc_I2c;
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

  at[0] = (t[8] << 8) + t[9];
  at[0] = ((1852 - at[0]) / 9.05) + 27.2;

  //x = x / 256000;
  //y = y / 256000;
  //z = z / 256000;
  x[0] = x[0] / 128000;
  y[0] = y[0] / 128000;
  z[0] = z[0] / 128000;
  
  if (t[10] == 5){//switching data to acc1,acc2
    x[2]=x[0];
    y[2]=y[0];
    z[2]=z[0];
    at[2]=at[0];
    }
  else{
    x[1]=x[0];
    y[1]=y[0];
    z[1]=z[0];
    at[1]=at[0];
    }   
}
void acc2_data() {
  t[10] = 5;
  acc_data();
  t[10] = 0;
  }
String nextLogFile(void)
{
  String filename;
  int logn = 0;

  for (int i = 0; i < 999; i++) {
    // Construct a file with PREFIX[Index].SUFFIX
    filename = String("log") + String(logn) + String(".txt");
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
  time = (millis() - t0) * 0.001;
  //Serial.println(time,3);
  //Serial.print("    i= ");
  //Serial.println(i);
  i++;
  if (i % 100 == 0) {
    logFile.close(); // close the file
    if (i % 1000 == 0) {
      f = i / time;
      Serial.print(F("freqency= ")); Serial.print(f); Serial.println(F(" Hz"));
      if (i > 0x30000)
        i = 0;
    }
  }
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
