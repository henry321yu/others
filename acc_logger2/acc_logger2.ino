#include <Arduino.h>
#include <SD.h>
#include <SPI.h>
#include <Wire.h>
#include <TimeLib.h>
#include "Watchdog_t4.h"

time_t RTCTime;
double setf = 0.1; /////////////設定頻率 0.1hz(18.75kb/hr) 1hz(187.5kb/hr) 30s=0.033hz(6.25kb/hr)
const int MPU_addr = 0x68;
long values[50];
String logdata, accdata, gyrdata, datadate, datedata, logFileName;
double x[3], y[3], z[3], gx[3], gy[3], gz[3], ta[3];
int ID, delayy, beeper = 13;
unsigned long i = 0;
String mon, dayy;
double timee, f, t0;
//int maxsize = 4718592;//4718592=4.5mb(~1day/1hz)=4.5*2^20 52428800=50mb=50*2^20(不超過excel)
//int maxsize = 460800;//460800=450kb(~1day/0.1hz)=450*2^10 52428800=50mb=50*2^20(不超過excel)
int maxsize = 76800;//76800=75kb(~4hr/0.1hz)=450*2^10 52428800=50mb=50*2^20
//int maxsize = 19200;//19200=18.75kb(~1hr/0.1hz)
//int maxsize = 1600;//1600=(~5min/0.1hz)
//int maxsize = 320;//320=(~1min/0.1hz)

const int SD_CS = BUILTIN_SDCARD;
File logFile;
WDT_T4<WDT3> wdt;

void setup() {
  Serial.begin(115200);
  delay(100);
  Serial.println(F("Serial.begin"));
  delay(100);

  while (!Serial && millis() < 4000 );
  Serial.println("\n" __FILE__ " " __DATE__ " " __TIME__);

  // RTC 設定
  setSyncProvider(getTeensy3Time);
  if (timeStatus() != timeSet) {
    Serial.println(F("RTC time not set! Rebooting..."));
    digitalWrite(beeper, HIGH);
    delay(1000);
    *((volatile uint32_t *)0xE000ED0C) = (0x5FA << 16) | (1 << 2);
  }

  Wire.begin();
  pinMode(beeper, OUTPUT);
  digitalWrite(beeper, LOW);

  // ------------------ SD卡初始化 ------------------
  Serial.println(F("SD.begin"));
  int sd_retry = 0;
  while (!SD.begin(SD_CS)) {
    Serial.println(F("SD card failed, or not present"));
    digitalWrite(beeper, HIGH);
    delay(300);
    digitalWrite(beeper, LOW);
    delay(700);
    sd_retry++;
    if (sd_retry >= 3) {
      Serial.println(F("SD init failed too many times, rebooting..."));
      digitalWrite(beeper, HIGH);
      delay(1000);
      *((volatile uint32_t *)0xE000ED0C) = (0x5FA << 16) | (1 << 2);
    }
  }

  // ------------------ 檔案建立 ------------------
  logFileName = nextLogFile_date_v2();
  Serial.println(logFileName);
  logFile = SD.open(logFileName.c_str(), FILE_WRITE);
  if (!logFile) {
    Serial.println(F("File open failed! Rebooting..."));
    digitalWrite(beeper, HIGH);
    delay(1000);
    *((volatile uint32_t *)0xE000ED0C) = (0x5FA << 16) | (1 << 2);
  }

  // ------------------ MPU6050 初始化 ------------------
  writeRegister(MPU_addr, 0x6B, 0x80); // reset
  delay(100);
  writeRegister(MPU_addr, 0x6B, 0x00); // wake up
  delay(30);

  // 嘗試讀取 MPU6050，確認是否回應
  Wire.beginTransmission(MPU_addr);
  byte mpuStatus = Wire.endTransmission();
  if (mpuStatus != 0) {
    Serial.println(F("MPU6050 not responding! Rebooting..."));
    digitalWrite(beeper, HIGH);
    delay(1000);
    *((volatile uint32_t *)0xE000ED0C) = (0x5FA << 16) | (1 << 2);
  }

  // ------------------ 設定其他參數 ------------------
  setf = 1 / setf * 1000;

  WDT_timings_t config;
  config.timeout = 30000;
  wdt.begin(config);

  Serial.println(F("Initialization done"));
  // 音效提示完成
  digitalWrite(beeper, HIGH);
  delay(100);
  digitalWrite(beeper, LOW);
  delay(500);
  digitalWrite(beeper, HIGH);
  delay(100);
  digitalWrite(beeper, LOW);
  delay(2000);
  t0 = millis() * 0.001;
}

void loop() {
  wdt.feed(); // reset watchdog

  //timer
  timer_v3();

  imu6050_data();
  accdata = String(x[0], 5) + ", " + String(y[0], 5) + ", " + String(z[0], 5);
  datedata = String(hour()) + ":" +  String(minute()) + ":" +  String(second());

  logdata = String(timee, 3) + ", " + datedata + "," + accdata + ", " + String(ta[0], 2);

  Serial.println(logdata);

  savedata();

  delay(setf);
}

void imu6050_data() {
  ID = MPU_addr;

  readmultiRegister(0x3B, 14);

  x[0] = values[0] << 8 | values[1];
  y[0] = values[2] << 8 | values[3];
  z[0] = values[4] << 8 | values[5];
  ta[0] = values[6] << 8 | values[7];
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

  ta[0] = (ta[0] / 340) - 160.87 + 3.63;
  if (ta[0] <= -90)
    ta[0] += 192.67;

}
void writeRegister(int ID, int reg, int data)
{
  Wire.beginTransmission(ID);
  Wire.write(reg);
  Wire.write(data);
  Wire.endTransmission();
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
  timee = timee - t0;
  f = i / timee;

  if (i % 12 == 0) { //設每幾筆資料儲存至SD卡
    if (logFile.size() > (uint64_t)maxsize) {
      logFileName = nextLogFile_date_v2();
      Serial.println(logFileName);
      t0 = millis() * 0.001;
    }
    logFile.close(); // close the file
  }
}

void timer_v3() {
  i++;
  timee = millis() * 0.001;
  timee = timee - t0;
  f = i / timee;

  if (i % 12 == 0) {
    if (logFile && logFile.size() > (uint64_t)maxsize) {
      logFile.flush();
      logFile.close();
      logFileName = nextLogFile_date_v2();
      Serial.println("Switching to new file: " + logFileName);
      t0 = millis() * 0.001;
      timee = 0;
    } else if (logFile) {
      logFile.flush(); // 確保寫入
      logFile.close(); // 強制釋放記憶體
    }
  }
}

time_t getTeensy3Time() {
  return Teensy3Clock.get();
}

String nextLogFile_date_v2(void) {
  mon = String(month());
  dayy = String(day());
  if (mon.length() < 2) {
    mon = "0" + mon;
  }
  if (dayy.length() < 2) {
    dayy = "0" + dayy;
  }
  datadate = mon + dayy;
  String filename;
  int logn = 0;
  for (int i = 0; i < 999; i++) {
    filename = datadate + "_" + String(logn) + String(".csv");
    if (!SD.exists(filename.c_str()))    {
      return filename;
    }
    logn++;
  }
  return "";
}

void savedata() {
  if (!logFile) {
    logFile = SD.open(logFileName.c_str(), FILE_WRITE);
  }
  if (logFile) {
    logFile.println(logdata);
    digitalWriteFast(beeper, HIGH);
  }
  else {
    Serial.println(F("error opening file"));
    digitalWriteFast(beeper, LOW);
  }
}
