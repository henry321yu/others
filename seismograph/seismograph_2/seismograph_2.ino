#include <Arduino.h>
#include <SD.h>
#include <SPI.h>
#include <Wire.h>
#include <TimeLib.h>
#include "Watchdog_t4.h"

#define MPU_ADDR 0x68
#define SAMPLE_HZ 200
#define SAMPLE_INTERVAL_MS (1000 / SAMPLE_HZ)
#define THRESHOLD_G 0.03
#define SD_CS BUILTIN_SDCARD
#define MAX_STORAGE_BYTES (uint64_t)(12.0 * 1024 * 1024 * 1024)  // 12GB

WDT_T4<WDT3> wdt;
float ax, ay, az, int_ax, int_ay, int_az;
long rawData[6];
unsigned long last_sample_time = 0;
unsigned long last_file_switch_time = 0;
unsigned long triggerEndTime = 0;
bool triggered = false;
String currentTempFile = "";
int beeper = 13;
String currentPermFile = "";  // 儲存目前的 perm 檔案名

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
  delay(1000);
  readMPU();
  int_ax = ax;
  int_ay = ay;
  int_az = az;
  
  switchTempLogFile();  // 產生初始 temp 檔案
  Serial.println("System ready.");
}

void loop() {
  wdt.feed();

  if (millis() - last_sample_time >= SAMPLE_INTERVAL_MS) {
    last_sample_time = millis();

    readMPU();
    float magnitude = sqrt((ax - int_ax) * (ax - int_ax) + (ay - int_ay) * (ay - int_ay) + (az - int_az) * (az - int_az));
    String data = timeStamp() + "," + String(ax, 5) + "," + String(ay, 5) + "," + String(az, 5) + "," + String(magnitude, 5);
    Serial.printf("%.5f,%.5f,%.5f,%.5f\n", ax, ay, az, magnitude*100);

    // 寫入 temp 檔案
    File f = SD.open(currentTempFile.c_str(), FILE_WRITE);
    if (f) {
      f.println(data);
      f.close();
    }

    // 若超過閾值則啟動觸發記錄
    if (magnitude > THRESHOLD_G) {
      if (!triggered) {
        triggered = true;
        triggerEndTime = millis() + 5UL * 60 * 1000;  // 3 分鐘
        currentPermFile = "perm_" + nextLogFileName();  // 只產生一次檔名
        Serial.println("⚠️ Trigger detected! Saving for 1 minutes...");
        Serial.println("🔒 Saving to: " + currentPermFile);
        digitalWrite(beeper, HIGH);
      }
    }

    // 在觸發期間內持續寫入同一個 perm 檔案
    if (triggered && millis() <= triggerEndTime) {
      File pf = SD.open(currentPermFile.c_str(), FILE_WRITE);
      if (pf) {
        pf.println(data);
        pf.close();
      }
    }

    // 觸發結束時關閉蜂鳴器
    if (triggered && millis() > triggerEndTime) {
      triggered = false;
      digitalWrite(beeper, LOW);
    }

    // 每 5 分鐘更換 temp 檔案
    if (millis() - last_file_switch_time >= 10UL * 60 * 1000) {
      switchTempLogFile();
    }

    // 每次取樣後檢查儲存空間
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
    String name = base + "_" + String(i) + ".txt";
    if (!SD.exists(("perm_" + name).c_str())) return name;
  }
  return "error.txt";
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
    if (!entry.isDirectory()) {
      total += entry.size();
    }
    entry.close();
  }

  if (total >= MAX_STORAGE_BYTES) {
    Serial.println("⚠️ 超過 12GB，刪除最舊 temp 檔案...");
    deleteOldestTempLog();
  }
}

void deleteOldestTempLog() {
  File root = SD.open("/");
  String oldestFile = "";
  int oldestIndex = 9999;

  while (true) {
    File entry = root.openNextFile();
    if (!entry) break;

    String name = entry.name();
    if (!entry.isDirectory() && name.startsWith("temp_") && name.endsWith(".txt")) {
      // 檔名格式：temp_MMDD_N.txt
      int underscoreIndex = name.indexOf('_', 5);  // 從第5個字元後找第二個底線
      int dotIndex = name.indexOf(".txt");
      if (underscoreIndex > 0 && dotIndex > underscoreIndex) {
        String indexStr = name.substring(underscoreIndex + 1, dotIndex);
        int index = indexStr.toInt();
        if (index < oldestIndex) {
          oldestIndex = index;
          oldestFile = name;
        }
      }
    }
    entry.close();
  }

  if (oldestFile != "") {
    SD.remove(oldestFile.c_str());
    Serial.println("🗑️ 刪除最舊 temp 檔案：" + oldestFile);
  } else {
    Serial.println("⚠️ 找不到可刪除的 temp_ 檔案");
  }
}
