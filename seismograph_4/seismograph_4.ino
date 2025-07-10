#include <Arduino.h>
#include <SD.h>
#include <SPI.h>
#include <Wire.h>
#include <TimeLib.h>
#include "Watchdog_t4.h"
#include <SoftwareSerial.h> //HC12(mcu's rx、hc-12's tx,   mcu's tx、hc-12's rx) 
#include "register.h"

#define SAMPLE_HZ 200
#define SAMPLE_INTERVAL_MS (1000 / SAMPLE_HZ)
#define THRESHOLD_G 0.03
#define SD_CS BUILTIN_SDCARD
#define MAX_STORAGE_BYTES (uint64_t)(25.0 * 1024 * 1024 * 1024)  // 12GB
#define PRE_TRIGGER_SECONDS 30
#define BUFFER_SIZE (SAMPLE_HZ * PRE_TRIGGER_SECONDS)
#define SETT 1
//#define SETT 0.083 //for testing

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
int beeper = 13;
String currentPermFile = "";  // 儲存目前的 perm 檔案名
String preTriggerBuffer[BUFFER_SIZE];
int bufferIndex = 0;
bool preTriggerSaved = false;
int ID;
const int acc_I2c = 0x1D;
long values[10], t[12];
float x[3], y[3], z[3], ta[3], atemp;
String event = "untrigger";
int events = 0;

void setup() {
  WDT_timings_t config;
  config.timeout = 30000;
  wdt.begin(config);

  pinMode(beeper, OUTPUT);
  digitalWrite(beeper, HIGH);
  delay(200);
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

  Serial.println("System ready.");
  digitalWrite(beeper, HIGH);
  delay(100);
  digitalWrite(beeper, LOW);
  delay(500);
  digitalWrite(beeper, HIGH);
  delay(100);
  digitalWrite(beeper, LOW);
  delay(2000);
  switchTempLogFile();  // 產生初始 temp 檔案
}

void loop() {
  wdt.feed();

  float error_acc = sqrt(ax * ax + ay * ay + az * az);
  while (error_acc == 0);

  if (millis() - last_sample_time >= SAMPLE_INTERVAL_MS) {
    last_sample_time = millis();

    acc_data();
    float magnitude = sqrt((ax - int_ax) * (ax - int_ax) + (ay - int_ay) * (ay - int_ay) + (az - int_az) * (az - int_az));
    String data = timeStamp() + "," + String(ax, 5) + "," + String(ay, 5) + "," + String(az, 5) + "," + String(magnitude, 5) + "," + String(events) + "," + event;
//    Serial.println(data);
    HC12.println(data);

    // 儲存到 circular buffer
    preTriggerBuffer[bufferIndex] = data;
    bufferIndex = (bufferIndex + 1) % BUFFER_SIZE;

    // 寫入 temp 檔案
    if (!f && !triggered) {
      f = SD.open(currentTempFile.c_str(), FILE_WRITE);
    }
    if (f && !triggered) {
      f.println(data);
      f.close();
      event = "untrigger";
    }


    // 若超過閾值則啟動觸發記錄
    if (magnitude > THRESHOLD_G) {
      f.close();
      if (!triggered) {
        events += 1;
        event = "triggered";
        triggered = true;
        preTriggerSaved = false;  // 尚未儲存 pre-trigger 資料
        triggerEndTime = millis() + 3UL * 60 * 1000 * SETT;  // ➜ 後 3 分鐘
        currentPermFile = "perm_" + nextLogFileName_perm();
        Serial.println(String(magnitude, 5));
        Serial.println("⚠ Trigger detected! Saving 30s before + 3 min after...");
        Serial.println("🔒 Saving to: " + currentPermFile);
        digitalWrite(beeper, HIGH);
      }
    }

    // 觸發狀態下寫入 perm 檔案
    if (triggered) {
      if (magnitude > THRESHOLD_G) {
        Serial.println(String(magnitude, 5));
        Serial.println("⚠ More trigger detected! add 1 more min...");
        triggerEndTime = millis() + 1UL * 60 * 1000 * SETT;  // ➜ 再加 1 分鐘
      }
      if (!pf)
        pf = SD.open(currentPermFile.c_str(), FILE_WRITE);
      // 第一次觸發後先儲存過去 1 分鐘資料
      if (!preTriggerSaved) {
        if (pf) {
          for (int i = 0; i < BUFFER_SIZE; i++) {
            int index = (bufferIndex + i) % BUFFER_SIZE;
            if (preTriggerBuffer[index].length() > 0)
              pf.println(preTriggerBuffer[index]);
          }
          pf.close();
          preTriggerSaved = true;
          Serial.println("✅ PreDataSaved.");
          Serial.println("Saving next 3 min data...");
        }
      }

      // 觸發期間持續寫入
      if (millis() <= triggerEndTime) {
        if (!pf)
          pf = SD.open(currentPermFile.c_str(), FILE_WRITE);
        if (pf) {
          pf.println(data);
          pf.close();
        }
      }

      // 觸發結束
      if (millis() >= triggerEndTime) {
        triggered = false;
        digitalWrite(beeper, LOW);
        Serial.println("✅ Trigger recording completed.");
      }
    }

    // 每 5 分鐘更換 temp 檔案
    if (millis() - last_file_switch_time >= 5UL * 60 * 1000 * SETT && !triggered) {
      switchTempLogFile();
    }
  }
}

// ===== 時間與檔名處理 =====
String twoDigit(int num) {
  return (num < 10 ? "0" : "") + String(num);
}

String timeStamp() {
  static uint32_t lastMillis = millis();
  static time_t lastSecond = now();

  uint32_t nowMillis = millis();
  if (nowMillis - lastMillis >= 1000) {
    lastSecond += 1;
    lastMillis += 1000;
  }

  int ms = nowMillis - lastMillis;
  char buf[32];
  sprintf(buf, "%04d-%02d-%02d %02d:%02d:%02d.%03d",
          year(lastSecond), month(lastSecond), day(lastSecond),
          hour(lastSecond), minute(lastSecond), second(lastSecond),
          ms);
  return String(buf);
}


time_t getTeensy3Time() {
  return Teensy3Clock.get();
}

String nextLogFileName() {
  // 關閉目前檔案
  if (f) {
    f.close();
  }
  String base = twoDigit(month()) + twoDigit(day());
  for (int i = 0; i < 9999; i++) {
    String name = base + "_" + String(i) + ".txt";
    if (!SD.exists(("temp_" + name).c_str())) return name;
  }
  return "error.txt";
}
String nextLogFileName_perm() {
  // 關閉目前檔案
  if (pf) {
    pf.close();
  }
  String base = twoDigit(month()) + twoDigit(day());
  for (int i = 0; i < 9999; i++) {
    String name = base + "_" + String(i) + ".txt";
    if (!SD.exists(("perm_" + name).c_str())) return name;
  }
  return "error.txt";
}

void switchTempLogFile() {
  // 檢查儲存空間
  checkStorageLimit();

  currentTempFile = "temp_" + nextLogFileName();
  last_file_switch_time = millis();
  Serial.println("New temp file: " + currentTempFile);
  Serial.println("Saving next temp log file...");
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
    Serial.println("⚠ 超過 12GB，刪除最舊 temp 檔案...");
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
