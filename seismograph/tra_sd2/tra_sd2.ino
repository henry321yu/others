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

  Serial.println("READY");
}

void sendFileList() {
  File root = SD.open("/");
  if (!root) {
    Serial.println("ROOT_FAIL");
    return;
  }

  int fileCount = 0;

  // 第一輪：計數
  File entry = root.openNextFile();
  while (entry) {
    if (!entry.isDirectory()) {
      fileCount++;
    }
    entry.close();
    entry = root.openNextFile();
  }

  Serial.print("FILE_COUNT:");
  Serial.println(fileCount);

  // 重新掃描列出檔案
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

void loop() {

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
