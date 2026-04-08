#include <SPI.h>
#include <SD.h>

const int chipSelect = 10;

#define BUFFER_SIZE 4096
#define ACK_TIMEOUT 5000

uint8_t buffer[BUFFER_SIZE];

String inputLine = "";
bool startTransfer = false;

bool waitForResponse(String expected) {
  unsigned long start = millis();
  String line = "";

  while (millis() - start < ACK_TIMEOUT) {
    while (Serial.available()) {
      char c = Serial.read();

      if (c == '\n') {
        line.trim();
        if (line == expected) return true;
        line = "";
      } else {
        line += c;
      }
    }
  }
  return false;
}

void setup() {
  Serial.begin(921600);
  while (!Serial);

  if (!SD.begin(chipSelect)) {
    Serial.println("SD_FAIL");
    while (1);
  }
}

void loop() {

  // ===== Session Reset / Command =====
  if (Serial.available()) {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();

    if (cmd == "HELLO") {
      Serial.println("READY");
      startTransfer = false;
      return;
    }

    if (cmd == "START") {
      startTransfer = true;
    }
  }

  if (!startTransfer) {
    Serial.println("READY");
    delay(1000);
    return;
  }

  // ===== List files =====
  File root = SD.open("/");
  if (!root) {
    Serial.println("ROOT_FAIL");
    startTransfer = false;
    return;
  }

  int fileCount = 0;
  uint32_t totalSize = 0;

  File f;

  while (true) {
    f = root.openNextFile();
    if (!f) break;

    if (!f.isDirectory()) {
      fileCount++;
      totalSize += f.size();
    }
    f.close();
  }

  Serial.print("FILE_COUNT:");
  Serial.println(fileCount);

  Serial.print("TOTAL_SIZE:");
  Serial.println(totalSize);

  root.rewindDirectory();

  while (true) {
    f = root.openNextFile();
    if (!f) break;

    if (!f.isDirectory()) {
      Serial.print("FILE:");
      Serial.print(f.name());
      Serial.print(",");
      Serial.println(f.size());
    }
    f.close();
  }

  Serial.println("END_LIST");

  // ===== Wait CONFIRM =====
  if (!waitForResponse("CONFIRM")) {
    startTransfer = false;
    return;
  }

  // ===== Transfer =====
  root.rewindDirectory();

  while (true) {
    File entry = root.openNextFile();
    if (!entry) break;

    if (entry.isDirectory()) {
      entry.close();
      continue;
    }

    const char* filename = entry.name();
    uint32_t fileSize = entry.size();

    // ===== Ask PC if skip =====
    Serial.print("FILE:");
    Serial.print(filename);
    Serial.print(",");
    Serial.println(fileSize);

    String response = "";
    unsigned long start = millis();

    while (millis() - start < ACK_TIMEOUT) {
      if (Serial.available()) {
        char c = Serial.read();
        if (c == '\n') {
          response.trim();
          break;
        } else {
          response += c;
        }
      }
    }

    if (response == "SKIP") {
      continue;
    }

    // ===== Send file =====
    Serial.print("SIZE:");
    Serial.println(fileSize);

    while (entry.available()) {
      int n = entry.read(buffer, BUFFER_SIZE);
      Serial.write(buffer, n);
    }

    Serial.println();
    Serial.println("END_FILE");

    // Wait ACK
    if (!waitForResponse("ACK")) {
      Serial.println("READY");
      startTransfer = false;
      return;
    }

    entry.close();
  }

  Serial.println("DONE");
  startTransfer = false;
}
