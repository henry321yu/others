#include <SPI.h>
#include <SD.h>

//const int chipSelect = 10;
const int chipSelect = BUILTIN_SDCARD;

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
    delay(3000);
    Serial.println("READY");
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
      char filename[256];

      if (!safeCopyFilename(f, filename, sizeof(filename))) {
        // skip corrupted entry silently
        f.close();
        continue;
      }

      Serial.print("FILE:");
      Serial.print(filename);
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

    char filename[256];  // 足夠 buffer

    if (!safeCopyFilename(entry, filename, sizeof(filename))) {
      entry.close();
      continue;
    }

    uint32_t fileSize = entry.size();

    if (!isValidFilename(filename)) {
      if (strlen(filename) == 0 || strlen(filename) > 255) {
        Serial.println("SKIP");
        entry.close();
        continue;
      }
      Serial.print("FILE:");
      Serial.print(filename);
      Serial.print(",");
      Serial.println(fileSize);

      Serial.println("SKIP");

      entry.close();
      continue;
    }

    // ===== Ask PC if skip =====
    Serial.print("FILE:");
    Serial.print(filename);
    Serial.print(",");
    Serial.println(fileSize);
    Serial.flush();

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
    Serial.flush();

    while (entry.available()) {
      int n = entry.read(buffer, BUFFER_SIZE);
      Serial.write(buffer, n);
      Serial.flush();
    }

    // Wait ACK
    if (!waitForResponse("ACK")) {
      delay(3000);
      Serial.println("READY");
      startTransfer = false;
      return;
    }
    entry.close();
  }

  Serial.println("DONE");
  startTransfer = false;
}

bool isValidFilename(const char* name) {
  if (name == nullptr) return false;

  for (int i = 0; name[i] != '\0'; i++) {
    char c = name[i];

    // 只允許可列印 ASCII
    if (c < 32 || c > 126) {
      return false;
    }
  }
  return true;
}
bool safeCopyFilename(File &f, char* out, size_t outSize) {
  const char* name = f.name();
  if (!name) return false;

  size_t len = strlen(name);
  if (len == 0 || len >= outSize) return false;

  // copy
  strncpy(out, name, outSize - 1);
  out[outSize - 1] = '\0';

  // ASCII check
  for (size_t i = 0; i < len; i++) {
    char c = out[i];
    if (c < 32 || c > 126) {
      return false;
    }
  }

  return true;
}
