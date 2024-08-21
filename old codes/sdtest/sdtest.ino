#include <SdFat.h>

// SD_FAT_TYPE = 0 for sd/File as defined in sdConfig.h,
// 1 for FAT16/FAT32, 2 for exFAT, 3 for FAT16/FAT32 and exFAT.
#define SD_FAT_TYPE 3

// Interval between data records in microseconds.
// Try 250 with Teensy 3.6, Due, or STM32.
// Try 2000 with AVR boards.
// Try 4000 with SAMD Zero boards.
const uint32_t LOG_INTERVAL_USEC = 250;


// Select the fastest interface. Assumes no other SPI devices.
#if ENABLE_DEDICATED_SPI
#define SD_CONFIG SdSpiConfig(sd_CS, DEDICATED_SPI)
#else  // ENABLE_DEDICATED_SPI
#define SD_CONFIG SdSpiConfig(sd_CS, SHARED_SPI)
#endif  // ENABLE_DEDICATED_SPI


#if SD_FAT_TYPE == 0
typedef SdFat sd_t;
typedef File file_t;
#elif SD_FAT_TYPE == 1
typedef SdFat32 sd_t;
typedef File32 file_t;
#elif SD_FAT_TYPE == 2
typedef SdExFat sd_t;
typedef ExFile file_t;
#elif SD_FAT_TYPE == 3
typedef SdFs sd_t;
typedef FsFile file_t;
#else  // SD_FAT_TYPE
#error Invalid SD_FAT_TYPE
#endif  // SD_FAT_TYPE

sd_t sd;

file_t logFile;

const int sd_CS = SS; // teensy builtin
String logFileName;
int led = 13;

void setup() {
  Serial.begin(115200);

  if (!sd.begin(SD_CONFIG)) {
  //if (!sd.begin(sd_CS)) {
    Serial.println(F("Card failed, or not present"));
  }
  pinMode(sd_CS, OUTPUT);
  logFileName = nextLogFile();

  logFile = sd.open(logFileName, FILE_WRITE);
  if (logFile) {
    Serial.println(F("writing"));
  }
  // if the file didn't open, print an error:
  else {
    Serial.println(F("error opening file"));
    //return;
  }
  pinMode(led, OUTPUT);
  digitalWrite(led, HIGH);
  delay(2000);
  digitalWrite(led, LOW);
  delay(3000);
}
void loop() {
    if (!logFile) {
    logFile = sd.open(logFileName, FILE_WRITE);
  }
  if (logFile) {
    digitalWrite(led, HIGH);
    logFile.println("aaa");
  }
  else {
    Serial.println(F("error opening test.txt"));
  }
  digitalWrite(led, LOW);
  delay(1000);
  }

  
String nextLogFile(void) {
  String filename;
  int logn = 0;
  for (int i = 0; i < 999; i++) {
    // Construct a file with PREFIX[Index].SUFFIX
    filename = String("log") + String(logn) + String(".txt");
    // If the file name doesn't exist, return it
    if (!sd.exists(filename))
    {
      return filename;
    }
    // Otherwise increment the index, and try again
    logn++;
  }
  return "";
}
