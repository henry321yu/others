#include <Arduino.h>
#include <SD.h>
#include <SPI.h>
#include <Wire.h>
#include <TimeLib.h>
#include "Watchdog_t4.h"

#define MPU_ADDR 0x68
#define SAMPLE_HZ 200
#define SAMPLE_INTERVAL_MS (1000 / SAMPLE_HZ)
//#define THRESHOLD_G 0.0102
#define THRESHOLD_G 0.2
#define SD_CS BUILTIN_SDCARD
#define MAX_STORAGE_BYTES (uint64_t)(12.0 * 1024 * 1024 * 1024)  // 12GB

WDT_T4<WDT3> wdt;
float ax, ay, az;
long rawData[6];
unsigned long last_sample_time = 0;
unsigned long last_file_switch_time = 0;
bool triggered = false;
String currentTempFile = "";
int beeper = 13;

void setup() {
  WDT_timings_t config;
  config.timeout = 30000;
  wdt.begin(config);

  pinMode(beeper, OUTPUT);
  digitalWrite(beeper, LOW);

  Serial.begin(115200);
  delay(100);
  while (!Serial && millis() < 4000);

  Serial.println("Initializing...");

  setSyncProvider(getTeensy3Time);
  if (timeStatus() != timeSet) {
    Serial.println("RTC time not set!");
  }

  Wire.begin();
  initMPU();

  if (!SD.begin(SD_CS)) {
    Serial.println("SD card initialization failed!");
    while (1);
  }

  switchTempLogFile();  // 產生初始檔案
  Serial.println("System ready.");
}

void loop() {
  wdt.feed();

  if (millis() - last_sample_time >= SAMPLE_INTERVAL_MS) {
    last_sample_time = millis();

    readMPU();
    float magnitude = sqrt(ax * ax + ay * ay + az * az) - 1;
    String data = timeStamp() + "," + String(ax, 5) + "," + String(ay, 5) + "," + String(az, 5);
//    Serial.printf("%.5f,%.5f,%.5f,%.5f\n", ax, ay, az, magnitude);

    // 寫入目前暫存檔案
    File f = SD.open(currentTempFile.c_str(), FILE_WRITE);
    if (f) {
      f.println(data);
      f.close();
    }

    // 若觸發事件，寫入永久檔案
    if (magnitude > THRESHOLD_G) {
      if (!triggered) {
        triggered = true;
        Serial.println("Trigger detected!");
        digitalWrite(beeper, HIGH);
      }

      String permFile = "perm_" + nextLogFileName();
      File pf = SD.open(permFile.c_str(), FILE_WRITE);
      if (pf) {
        pf.println(data);
        pf.close();
      }
    }
    else {
      triggered = false;
    }

    // 每 n 分鐘更換一次 temp 檔名
    if (millis() - last_file_switch_time >= 1UL * 60 * 1000) {
      switchTempLogFile();
      digitalWrite(beeper, LOW);
    }

    // 每次也檢查儲存空間
    checkStorageLimit();
  }
}

// ===== MPU 設定與讀取 =====
void initMPU() {
  writeRegister(MPU_ADDR, 0x6B, 0x80); delay(100);
  writeRegister(MPU_ADDR, 0x6B, 0x00); delay(100);
}

void writeRegister(int dev, int reg, int data) {
  Wire.beginTransmission(dev);
  Wire.write(reg);
  Wire.write(data);
  Wire.endTransmission();
}

void readMPU() {
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x3B);
  Wire.endTransmission(false);
  Wire.requestFrom(MPU_ADDR, 6);

  for (int i = 0; i < 3; i++) {
    rawData[i] = (Wire.read() << 8) | Wire.read();
    if (rawData[i] >= 0x8000) rawData[i] -= 0x10000;
  }

  ax = rawData[0] / 16384.0;
  ay = rawData[1] / 16384.0;
  az = rawData[2] / 16384.0;
}

// ===== 時間與檔名處理 =====
String twoDigit(int num) {
  return (num < 10 ? "0" : "") + String(num);
}

String timeStamp() {
  char ms[4];
  sprintf(ms, "%03d", (int)(millis() % 1000));
  return String(year()) + "-" + twoDigit(month()) + "-" + twoDigit(day()) + " " +
         twoDigit(hour()) + ":" + twoDigit(minute()) + ":" + twoDigit(second()) + "." + ms;
}

time_t getTeensy3Time() {
  return Teensy3Clock.get();
}

String nextLogFileName() {
  String base = twoDigit(month()) + twoDigit(day());
  for (int i = 0; i < 9999; i++) {
    String name = base + "_" + String(i) + ".csv";
    if (!SD.exists(name.c_str())) return name;
  }
  return "error.csv";
}

void switchTempLogFile() {
  currentTempFile = "temp_" + nextLogFileName();
  last_file_switch_time = millis();
  Serial.println("📝 New temp file: " + currentTempFile);
}

// ===== 空間與檔案管理 =====
void checkStorageLimit() {
  uint64_t total = 0;
  File root = SD.open("/");
  while (true) {
    File entry = root.openNextFile();
    if (!entry) break;
    if (!entry.isDirectory()) total += entry.size();
    entry.close();
  }

  if (total >= MAX_STORAGE_BYTES) {
    Serial.println("⚠️ SD 超過限制，刪除最舊 temp 檔案...");
    deleteOldestTempLog();
  }
}

void deleteOldestTempLog() {
  File root = SD.open("/");
  String oldestFile = "";
  uint32_t oldestTime = UINT32_MAX;

  while (true) {
    File entry = root.openNextFile();
    if (!entry) break;

    String name = entry.name();
    if (!entry.isDirectory() && name.startsWith("temp_")) {
      DateTimeFields dt;  // 定義時間結構體
      if (entry.getCreateTime(dt)) {  // 成功取得建立時間
        uint32_t timeValue = dt.year * 1000000 + dt.mon * 10000 + dt.wday * 100 + dt.hour; // 自訂可比較的整數值
        if (timeValue < oldestTime) {
          oldestTime = timeValue;
          oldestFile = name;
        }
      }
    }
    entry.close();
  }

  if (oldestFile != "") {
    SD.remove(oldestFile.c_str());
    Serial.println("🗑️ 刪除檔案：" + oldestFile);
  }
}
