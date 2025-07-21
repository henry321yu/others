#include <Arduino.h>
#include <SD.h>
#include <SPI.h>
#include <Wire.h>
#include <TimeLib.h>
#include "Watchdog_t4.h"
#include <SoftwareSerial.h> //HC12(mcu's rx、hc-12's tx,   mcu's tx、hc-12's rx) 
#include "register.h"

#define SAMPLE_HZ 100
#define SAMPLE_INTERVAL_MS (1000 / SAMPLE_HZ)
#define THRESHOLD_G 0.03
#define SD_CS 10
#define MAX_STORAGE_MEGABYTES 1860  // byte = mb*2^20  (100hz 38.75mb/hr )
#define PRE_TRIGGER_SECONDS 40
#define BUFFER_SIZE (SAMPLE_HZ * PRE_TRIGGER_SECONDS)
#define SETT 1
//#define SETT 0.1 //for testing

File f;
File pf;
SoftwareSerial HC12(21, 20); int setpin = 22; //small box

WDT_T4<WDT3> wdt;
float ax, ay, az, int_ax, int_ay, int_az;
long rawData[6];
unsigned long last_sample_time = 0;
unsigned long last_file_switch_time = 0;
unsigned long triggerEndTime = 0;
bool triggered = false;
String currentTempFile = "";
int beeper = 14;
String currentPermFile = "";  // 儲存目前的 perm 檔案名
String preTriggerBuffer[BUFFER_SIZE];
int bufferIndex = 0;
bool preTriggerSaved = false;
int ID;
const int acc_I2c = 0x1D;
long values[10], t[12];
float x[3], y[3], z[3], ta[3], atemp;
String statuss = "untrigger";
String nowfile = "";
int events = 0;
long i = 0;
float nowmillis;
int wdtt = 20000;

double tt[10], freq = SAMPLE_HZ;
long ii = 500; long iii = 1000;
int delayy = 1000, setF = SAMPLE_HZ;

bool trigger_pending = false;
unsigned long trigger_detected_time = 0;

void setup() {
  WDT_timings_t config;
  config.timeout = wdtt;
  wdt.begin(config);

  pinMode(beeper, OUTPUT);
  digitalWrite(beeper, HIGH);
  delay(200);
  digitalWrite(beeper, LOW);

  Serial.begin(115200);
  delay(100);

  Serial.println("Initializing...");

  //  Serial.println("" __FILE__ " " __DATE__ " " __TIME__);
  setSyncProvider(getTeensy3Time);
  if (timeStatus() != timeSet) {
    Serial.println("RTC time not set!");
  }

  Serial.printf("Watchdog timeout : %ds\n", wdtt / 1000);

  Wire.begin();
  Serial.println(F("HC12.reset"));  //115200
  pinMode(setpin, OUTPUT); digitalWrite(setpin, LOW); // for reset
  HC12.begin(9600);
  delay(100);
  HC12.print("AT+B115200");
  delay(100);
  Serial.println(F("HC12.begin and set"));
  HC12.begin(115200);
  delay(100);
  HC12.print("AT+B115200");
  delay(100);
  HC12.print("AT+C067"); //127 for imu //117 for mag sensor gps //112 for mag sensor //107 for gps//097 for rtk // 087 for rasp power
  delay(100);
  HC12.print("AT+P8");
  delay(100);
  digitalWrite(setpin, HIGH);
  Serial.println(F("HC12.set"));

  while (HC12.available()) {
    Serial.write(HC12.read());
  }

  //adxl355 setting
  writeRegister(acc_I2c, RESET, 0x52);  // reset sensor
  delay(100);
  writeRegister(acc_I2c, POWER_CTL, 0x00);  // writing 0 to to enable sensor
  delay(30);
  writeRegister(acc_I2c, RANGE, 0x01);
  delay(30);
  writeRegister(acc_I2c, SELF_TEST, 0x00);  // writing 3 to to enable self test
  delay(30);
  delay(500);

  if (!SD.begin(SD_CS)) {
    Serial.println("SD card initialization failed!");
    while (1);
  }
  delay(1000);
  acc_data();
  int_ax = ax;
  int_ay = ay;
  int_az = az;

  if (Serial) {
    Serial.println("SD卡檔案列表：");
    listSDContents();
  }

  Serial.println("產生初始 temp 檔案...");
  switchTempLogFile();
  nowfile = currentTempFile;
  Serial.println("System ready.");
  digitalWrite(beeper, HIGH);
  delay(100);
  digitalWrite(beeper, LOW);
  delay(500);
  digitalWrite(beeper, HIGH);
  delay(100);
  digitalWrite(beeper, LOW);
  delay(1000);
}

void loop() {
  wdt.feed();

  float error_acc = sqrt(ax * ax + ay * ay + az * az);
  while (error_acc == 0) {
    Serial.println("error_acc");
    HC12.println("error_acc");
    delay(1000);
  }

  nowmillis = millis();
  last_sample_time = nowmillis;

  acc_data();
  float magnitude = sqrt((ax - int_ax) * (ax - int_ax) + (ay - int_ay) * (ay - int_ay) + (az - int_az) * (az - int_az));
  String acc_String_data = String(ax, 5) + "," + String(ay, 5) + "," + String(az, 5) + "," + String(magnitude, 5) + "," + String(atemp, 3);
//  String data = String(nowmillis * 0.001, 3) + "," + timeStamp() + "," + acc_String_data + "," + String(events) + "," + statuss + "," + String(freq, 2) + "," + nowfile;
  String data = String(nowmillis * 0.001, 3) + "," + timeStamp() + "," + acc_String_data + "," + String(events) + "," + statuss + "," + String(freq, 2);

  //  Serial.println(data);
  HC12.println(data + "," + nowfile);

  if (!triggered) {
    // 儲存到 circular buffer
    preTriggerBuffer[bufferIndex] = data;
    bufferIndex = (bufferIndex + 1) % BUFFER_SIZE;
  }

  // 寫入 temp 檔案
  if (!f && !triggered) {
    f = SD.open(currentTempFile.c_str(), FILE_WRITE);
    if (!f) {
      Serial.println("⚠ 無法寫入 temp 檔案：" + currentTempFile);
    }
  }
  if (f && !triggered && !trigger_pending) {
    f.println(data);
    statuss = "untrigger";
    nowfile = currentTempFile;
  }


  // 若超過閾值則啟動觸發記錄
  if (!triggered && magnitude > THRESHOLD_G && !trigger_pending) {
    trigger_pending = true;
    trigger_detected_time = nowmillis;
    events += 1;
    triggerEndTime = nowmillis + 2UL * 60 * 1000 * SETT;
    statuss = "triggered";
    Serial.println("⚠ Trigger detected! Saving 40s before + 2 min after...");
    Serial.printf("End perm file at %ds...\n", triggerEndTime / 1000);
    digitalWrite(beeper, HIGH);
  }

  if (trigger_pending && (nowmillis - trigger_detected_time >= 15000)) { // 10s後再處裡 以防掉幀
    trigger_pending = false;
    triggered = true;
    preTriggerSaved = false;
    currentPermFile = "perm_" + nextLogFileName_perm();
    Serial.println("🔒 Saving to: " + currentPermFile);
    nowfile = currentPermFile;
  }

  // 觸發狀態下寫入 perm 檔案
  if (triggered) {
    if (magnitude > THRESHOLD_G) {
      Serial.println(String(magnitude, 5));
      Serial.println("⚠ More trigger detected! add 2 more min...");
      triggerEndTime = nowmillis + 2UL * 60 * 1000 * SETT;  // ➜ 再加 2 分鐘
      Serial.printf("End perm file at %ds...\n", triggerEndTime / 1000);
    }
    if (!pf) {
      pf = SD.open(currentPermFile.c_str(), FILE_WRITE);
      if (!pf) {
        Serial.println("⚠ 無法寫入 perm 檔案：" + currentPermFile);
      }
    }
    // 第一次觸發後先儲存過去 1 分鐘資料
    if (!preTriggerSaved) {
      if (pf) {
        for (int i = 0; i < BUFFER_SIZE; i++) {
          int index = (bufferIndex + i) % BUFFER_SIZE;
          if (preTriggerBuffer[index].length() > 0)
            pf.println(preTriggerBuffer[index]);
        }
        preTriggerSaved = true;
        Serial.println("✅ PreDataSaved.");
        Serial.println("Saving next 2 min data...");
      }
    }

    // 觸發期間持續寫入
    if (nowmillis <= triggerEndTime) {
      if (!pf) {
        pf = SD.open(currentPermFile.c_str(), FILE_WRITE);
        if (!pf) {
          Serial.println("⚠ 無法寫入 perm 檔案：" + currentPermFile);
        }
      }
      if (pf) {
        pf.println(data);
      }
    }

    // 觸發結束
    if (nowmillis >= triggerEndTime) {
      triggered = false;
      digitalWrite(beeper, LOW);
      Serial.println("✅ Trigger recording completed.");
      switchTempLogFile();
    }
  }

  // 每 10 分鐘更換 temp 檔案
  if (nowmillis - last_file_switch_time >= 10UL * 60 * 1000 * SETT && !triggered) {
    switchTempLogFile();
  }
  timer_v2();
  delayMicroseconds(delayy);
}

// ===== 時間與檔名處理 =====
String twoDigit(int num) {
  return (num < 10 ? "0" : "") + String(num);
}

String timeStamp() {
  time_t t = now();  // 使用 RTC 時間確保跨日正確
  int ms = ((unsigned long)nowmillis) % 1000;  // 修正餘數計算
  char buf[32];
  sprintf(buf, "%04d-%02d-%02d %02d:%02d:%02d.%03d",
          year(t), month(t), day(t),
          hour(t), minute(t), second(t), ms);
  return String(buf);
}

time_t getTeensy3Time() {
  return Teensy3Clock.get();
}

String nextLogFileName() {
  if (f) {
    f.flush();
    f.close();
  }
  if (pf) {
    pf.flush();
    pf.close();
  }

  String base = String(year() % 100) + twoDigit(month()) + twoDigit(day());
  int maxIndex = -1;

  File root = SD.open("/");
  while (true) {
    File entry = root.openNextFile();
    if (!entry) break;

    String name = entry.name();
    if (name.startsWith("temp_" + base)) {
      int underscoreIndex = name.indexOf('_', 5);
      int dotIndex = name.indexOf(".txt");
      if (underscoreIndex > 0 && dotIndex > underscoreIndex) {
        String indexStr = name.substring(underscoreIndex + 1, dotIndex);
        int index = indexStr.toInt();
        if (index > maxIndex) maxIndex = index;
      }
    }
    entry.close();
  }

  return base + "_" + String(maxIndex + 1) + ".txt";
}

String nextLogFileName_perm() {
  if (pf) {
    pf.flush();
    pf.close();
  }

  String base = String(year() % 100) + twoDigit(month()) + twoDigit(day());
  int maxIndex = -1;

  File root = SD.open("/");
  while (true) {
    File entry = root.openNextFile();
    if (!entry) break;

    String name = entry.name();
    if (name.startsWith("perm_" + base)) {
      int underscoreIndex = name.indexOf('_', 5);
      int dotIndex = name.indexOf(".txt");
      if (underscoreIndex > 0 && dotIndex > underscoreIndex) {
        String indexStr = name.substring(underscoreIndex + 1, dotIndex);
        int index = indexStr.toInt();
        if (index > maxIndex) maxIndex = index;
      }
    }
    entry.close();
  }

  return base + "_" + String(maxIndex + 1) + ".txt";
}


void switchTempLogFile() {
  // 檢查儲存空間
  checkStorageLimit();

  currentTempFile = "temp_" + nextLogFileName();
  last_file_switch_time = nowmillis;
  Serial.println("New temp file: " + currentTempFile);
  Serial.println("Saving next temp log file...");
}

// ===== 空間與檔案管理 =====
void checkStorageLimit() {
  unsigned long total = 0;
  float maxx = MAX_STORAGE_MEGABYTES;
  float mbb = (1 << 20);  // 2 的 20 次方 byte to mb
  maxx = maxx * mbb;
  File root = SD.open("/");
  while (true) {
    File entry = root.openNextFile();
    if (!entry) break;
    if (!entry.isDirectory()) {
      total += entry.size();
    }
    entry.close();
  }
  Serial.printf("目前檔案大小：%.2f mb ", total / mbb);

  if (total >= maxx) {
    Serial.printf("⚠ 超過設定的 %.2f mb，刪除最舊 temp 檔案...\n", maxx / mbb);
    deleteOldestTempLog();
  }
  else
    Serial.printf("小於設定的檔案限制 %.2f mb.繼續...\n", maxx / mbb);
}

void deleteOldestTempLog() {
  File root = SD.open("/");
  String oldestDate = "";
  int oldestIndex = 9999;
  String oldestFile = "";

  while (true) {
    File entry = root.openNextFile();
    if (!entry) break;

    String name = entry.name();
    if (!entry.isDirectory() && name.startsWith("temp_") && name.endsWith(".txt")) {
      // 檔名格式：temp_MMDD_N.txt
      int secondUnderscore = name.indexOf('_', 7);  // 找第二個底線
      int dotIndex = name.indexOf(".txt");
      if (secondUnderscore > 0 && dotIndex > secondUnderscore) {
        String dateStr = name.substring(5, secondUnderscore);          // 取得 MMDD
        String indexStr = name.substring(secondUnderscore + 1, dotIndex);
        int index = indexStr.toInt();

        // 比較日期
        if (oldestDate == "" || dateStr.toInt() < oldestDate.toInt()) {
          oldestDate = dateStr;
          oldestIndex = index;
          oldestFile = name;
        }
        // 如果同一天，選 index 較小的
        else if (dateStr == oldestDate && index < oldestIndex) {
          oldestIndex = index;
          oldestFile = name;
        }
      }
    }
    entry.close();
  }

  if (oldestFile != "") {
    SD.remove(oldestFile.c_str());
    Serial.println("刪除最舊 temp 檔案：" + oldestFile);
  } else {
    Serial.println("⚠ 找不到可刪除的 temp_ 檔案");
  }
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

void acc_data() {
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

  ta[0] = (t[8] << 8) + t[9];
  ta[0] = ((1852 - ta[0]) / 9.05);

  ax = x[0] / 256000;  //2G
  ay = y[0] / 256000;
  az = z[0] / 256000;

  atemp = ta[0] + 28.52;
}

void timer_v2() {
  i++;
  float timee = nowmillis * 0.001;

  if (i >= iii) {
    iii = i + 500;
    tt[1] = timee;
    tt[3] = tt[0];
    tt[2] = tt[1] - tt[0];
    freq = 500 / tt[2];
    if (f) f.flush();
    if (pf) pf.flush();

    if (abs(freq - setF) > 1) {
      if (setF > freq) {
        delayy -= (setF - freq) * 100;
      }
      if (setF < freq) {
        delayy += (freq - setF) * 100;
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
}
void listSDContents() {
  File root = SD.open("/");
  while (true) {
    File entry = root.openNextFile();
    if (!entry) break;
    Serial.print(entry.name());
    Serial.print(" - ");
    Serial.print(entry.size());
    Serial.println(" bytes");
    entry.close();
  }
}
