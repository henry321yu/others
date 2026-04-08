#include <SPI.h>
#include <SD.h>

const int chipSelect = 10;

#define BUFFER_SIZE 4096
uint8_t buffer[BUFFER_SIZE];

bool startTransfer = false;
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

void loop() {

  // ===== 讀取 PC 指令 =====
  while (Serial.available()) {
    char c = Serial.read();

    if (c == '\n') {
      inputLine.trim();

      if (inputLine == "START") {
        startTransfer = true;
      }

      inputLine = "";
    } else {
      inputLine += c;
    }
  }

  if (!startTransfer) return;

  // ===== 掃描 SD 卡 =====
  File root = SD.open("/");
  if (!root) {
    Serial.println("ROOT_FAIL");
    startTransfer = false;
    return;
  }

  File entry;

  while (true) {
    entry = root.openNextFile();
    if (!entry) break;

    // 忽略資料夾
    if (entry.isDirectory()) {
      entry.close();
      continue;
    }

    const char* filename = entry.name();

    Serial.print("FILE:");
    Serial.println(filename);

    uint32_t fileSize = entry.size();

    Serial.print("SIZE:");
    Serial.println(fileSize);

    // 傳輸檔案內容
    while (entry.available()) {
      int n = entry.read(buffer, BUFFER_SIZE);
      Serial.write(buffer, n);
    }

    entry.close();
  }

  Serial.println("DONE");
  startTransfer = false;
}
