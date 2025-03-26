#include <SD.h>
#include <SPI.h>
#include <Wire.h>
#include <Arduino.h>
#include <TimeLib.h>

time_t RTCTime;
const int MPU_addr = 0x68;
long values[50];
String logdata, accdata, gyrdata, datadate, logFileName;
double x[3], y[3], z[3], gx[3], gy[3], gz[3], ta[3];
int ID, delayy, beeper = 13;
unsigned long i = 0;
String mon, dayy;
double timee, f;
int maxsize = 209715200;//209715200=200mb=200*2^20;

//const int SD_CS = BUILTIN_SDCARD;
const int SD_CS = 10;
File logFile;

void setup() {
  Serial.begin(115200); // Initialize serial output via USB
  delay(100);
  Serial.println(F("Serial.begin"));
  delay(100);

  while (!Serial && millis() < 4000 );
  Serial.println("\n" __FILE__ " " __DATE__ " " __TIME__);
  setSyncProvider(getTeensy3Time);

  Serial.println(F("Wire.begin"));
  Wire.begin();

  pinMode(beeper, OUTPUT);
  Serial.println(F("SD.begin"));
  if (!SD.begin(SD_CS)) {
    Serial.println(F("Card failed, or not present"));
  }

  //  while (!SD.begin(SD_CS)) {
  //    Serial.println(F("Card failed, or not present"));
  //    digitalWrite(beeper, HIGH);
  //    delay(500);
  //    digitalWrite(beeper, LOW);
  //    delay(500);
  //  }

  Serial.println(F("file naming"));
  mon = String(month());
  dayy = String(day());
  if (mon.length() < 2) {
    mon = "0" + mon;
  }
  if (dayy.length() < 2) {
    dayy = "0" + dayy;
  }
  datadate = mon + dayy;
  logFileName = nextLogFile_date();

  logFile = SD.open(logFileName, FILE_WRITE);
  if (!logFile)
    Serial.println(F("error opening file"));

  //wakes up the MPU-6050
  writeRegister(MPU_addr, 0x6B, 0x80);// reset
  delay(100);
  writeRegister(MPU_addr, 0x6B, 0x00);// set to zero (wakes up the MPU-6050)
  delay(30);

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
  imu6050_data();
  accdata = String(x[0], 5) + ", " + String(y[0], 5) + ", " + String(z[0], 5);

  logdata = accdata + ", " + f;

  Serial.println(logdata);

  savedata();

  //timer
  timer_v2();

  delay(100);
}

void imu6050_data() {
  ID = MPU_addr;

  readmultiRegister(0x3B, 14);

  x[0] = values[0] << 8 | values[1];
  y[0] = values[2] << 8 | values[3];
  z[0] = values[4] << 8 | values[5];
  ta[10] = values[6] << 8 | values[7];
  gx[0] = values[8] << 8 | values[9];
  gy[0] = values[10] << 8 | values[11];
  gz[0] = values[12] << 8 | values[13];

  if (x[0] >= 0x8000) {
    x[0] = x[0] -  0x10000;
  }
  if (y[0] >= 0x8000) {
    y[0] = y[0] -  0x10000;
  }
  if (z[0] >= 0x8000) {
    z[0] = z[0] -  0x10000;
  }
  if (gx[0] >= 0x8000)
    gx[0] = gx[0] -  0x10000;
  if (gy[0] >= 0x8000)
    gy[0] = gy[0] -  0x10000;
  if (gz[0] >= 0x8000)
    gz[0] = gz[0] -  0x10000;

  x[0] = x[0] / 0x4000;
  y[0] = y[0] / 0x4000;
  z[0] = z[0] / 0x4000;

  gx[0] = gx[0] / 131;
  gy[0] = gy[0] / 131;
  gz[0] = gz[0] / 131;

  ta[0] = (ta[10] / 340) - 160.87 + 3.63;
}
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

void timer_v2() {
  i++;
  timee = millis() * 0.001;
  f = i / timee;

  if (i % 60 == 0) { //設每幾筆資料儲存至SD卡
    if (logFile.size() > maxsize) {
      logFileName = nextLogFile_date();
    }
    logFile.close(); // close the file
  }
}

time_t getTeensy3Time() {
  return Teensy3Clock.get();
}

String nextLogFile_date(void) {
  String filename;
  int logn = 0;
  for (int i = 0; i < 999; i++) {
    filename = datadate + "_" + String(logn) + String(".csv");
    if (!SD.exists(filename))    {
      return filename;
    }
    logn++;
  }
  return "";
}

void savedata() {
  if (!logFile) {
    logFile = SD.open(logFileName, FILE_WRITE);
  }
  if (logFile)
    logFile.println(logdata);
  else
    Serial.println(F("error opening file"));
}
