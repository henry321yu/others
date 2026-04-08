#include <SPI.h>
#include <SD.h>

const int chipSelect = 10;

#define BUFFER_SIZE 4096
uint8_t buffer[BUFFER_SIZE];

bool startTransfer = false;
bool listSent = false;

String inputLine = "";

void setup() {
  Serial.begin(921600);
  while (!Serial);

  if (!SD.begin(chipSelect)) {
    Serial.println("SD_FAIL");
    while (1);
  }
}

void loop() {

  // ===== 持續發送 READY（讓 PC 可隨時偵測）=====
  static unsigned long lastReady = 0;
  if (millis() - lastReady > 1000) {
    Serial.println("READY");
    lastReady = millis();
  }

  // ===== 讀取 PC 指令 =====
  while (Serial.available()) {
    char c = Serial.read();

    if (c == '\n') {
      inputLine.trim();

      if (inputLine == "START") {
        sendFileList();
      }

      if (inputLine == "CONFIRM") {
        startTransfer = true;
      }

      inputLine = "";
    } else {
      inputLine += c;
    }
  }

  if (!startTransfer) return;

  // ===== 傳輸檔案 =====
  File root = SD.open("/");
  File entry;

  while (true) {
    entry = root.openNextFile();
    if (!entry) break;

    if (entry.isDirectory()) {
      entry.close();
      continue;
    }

    Serial.print("FILE:");
    Serial.println(entry.name());

    Serial.print("SIZE:");
    Serial.println(entry.size());

    while (entry.available()) {
      int n = entry.read(buffer, BUFFER_SIZE);
      Serial.write(buffer, n);
    }

    entry.close();
  }

  Serial.println("DONE");
  startTransfer = false;
}

// ===== 列出檔案 =====
void sendFileList() {

  File root = SD.open("/");
  if (!root) {
    Serial.println("ROOT_FAIL");
    return;
  }

  uint32_t fileCount = 0;
  uint64_t totalSize = 0;

  // 第一輪：計數 + 總大小
  File entry = root.openNextFile();
  while (entry) {
    if (!entry.isDirectory()) {
      fileCount++;
      totalSize += entry.size();
    }
    entry.close();
    entry = root.openNextFile();
  }

  Serial.print("FILE_COUNT:");
  Serial.println(fileCount);

  Serial.print("TOTAL_SIZE:");
  Serial.println((uint32_t)totalSize);

  // 第二輪：列出檔案
  root.rewindDirectory();

  entry = root.openNextFile();
  while (entry) {
    if (!entry.isDirectory()) {
      Serial.print("FILE:");
      Serial.print(entry.name());
      Serial.print(",");
      Serial.println(entry.size());
    }
    entry.close();
    entry = root.openNextFile();
  }

  Serial.println("END_LIST");
}
