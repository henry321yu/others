#include "SdFat.h"
#include "sdios.h"
#include "FreeStack.h"

// SD_FAT_TYPE = 0 for SdFat/File as defined in SdFatConfig.h,
// 1 for FAT16/FAT32, 2 for exFAT, 3 for FAT16/FAT32 and exFAT.
#define SD_FAT_TYPE 2
/*
  Change the value of SD_CS_PIN if you are using SPI and
  your hardware does not use the default value, SS.
  Common values are:
  Arduino Ethernet shield: pin 4
  Sparkfun SD shield: pin 8
  Adafruit SD shields and modules: pin 10
*/
// SDCARD_SS_PIN is defined for the built-in SD on some boards.
#ifndef SDCARD_SS_PIN
const uint8_t SD_CS_PIN = SS;
#else  // SDCARD_SS_PIN
// Assume built-in SD is used.
const uint8_t SD_CS_PIN = SDCARD_SS_PIN;
#endif  // SDCARD_SS_PIN

// Try max SPI clock for an SD. Reduce SPI_CLOCK if errors occur.
#define SPI_CLOCK SD_SCK_MHZ(50)

// Try to select the best SD card configuration.
#if HAS_SDIO_CLASS
#define SD_CONFIG SdioConfig(FIFO_SDIO)
#elif ENABLE_DEDICATED_SPI
#define SD_CONFIG SdSpiConfig(SD_CS_PIN, DEDICATED_SPI, SPI_CLOCK)
#else  // HAS_SDIO_CLASS
#define SD_CONFIG SdSpiConfig(SD_CS_PIN, SHARED_SPI, SPI_CLOCK)
#endif  // HAS_SDIO_CLASS


#if SD_FAT_TYPE == 0
SdFat sd;
File file;
#elif SD_FAT_TYPE == 1
SdFat32 sd;
File32 file;
#elif SD_FAT_TYPE == 2
SdExFat sd;
ExFile file;
#elif SD_FAT_TYPE == 3
SdFs sd;
FsFile file;
#else  // SD_FAT_TYPE
#error Invalid SD_FAT_TYPE
#endif  // SD_FAT_TYPE

// Serial output stream
ArduinoOutStream cout(Serial);
//------------------------------------------------------------------------------
// Store error strings in flash to save RAM.
#define error(s) sd.errorHalt(&Serial, F(s))
//------------------------------------------------------------------------------

String logfileName = "";
String logdata;
double a = 1.229;
double b = 2.339;
double c = 5.449;

void setup() {
  Serial.begin(9600);

  // Wait for USB Serial
  while (!Serial) {
    SysCall::yield();
  }
  delay(1000);
  cout << F("\nUse a freshly formatted SD for best performance.\n");
  if (!ENABLE_DEDICATED_SPI) {
    cout << F(
           "\nSet ENABLE_DEDICATED_SPI nonzero in\n"
           "SdFatConfig.h for best SPI performance.\n");
  }

  // use uppercase in hex and use 0X base prefix
  cout << uppercase << showbase << endl;
}
//------------------------------------------------------------------------------
void loop() {
  logdata = String(a) + "," + String(b) + "," + String(c);


  // F() stores strings in flash to save RAM
  cout << F("Type any character to start\n");
  while (!Serial.available()) {
    SysCall::yield();
  }
  if (!sd.begin(SD_CONFIG)) {
    error("begin failed");
  }
  logfileName = nextLogFile();

  // open or create file - truncate existing file.
  if (!file) {
    file = sd.open(logfileName,FILE_WRITE);
    cout << F("opening\n");
  }
  if (file) {
    for (int i=0;i<100;i++){
    file.println(logdata);
    cout << F("writing\n");
  }
  }
  delay(2000);
  cout << endl << F("Done") << endl;
  file.close();
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
