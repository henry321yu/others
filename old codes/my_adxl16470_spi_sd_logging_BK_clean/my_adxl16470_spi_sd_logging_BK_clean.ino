#include <SPI.h>
#include <ADIS16470.h>
#include <SD.h>

const int SD_CS = 15;
const int acc16475_CS = 10; //
const int acc16475_DR = 2;
const int acc16475_RSET = 6;
double x, y, z, c;
int16_t regData[32];
byte accdataN = 24;
int32_t gxo, gyo, gzo, axo, ayo, azo = 0;
int32_t gxb, gyb, gzb, axb, ayb, azb = 0;
double ax, ay, az, gx, gy, gz;
byte regread0 , regread1, regread2, regread3;
String logdata, logFileName, accdata, gyrdata;
File logFile;

ADIS16470 IMU(acc16475_CS, acc16475_DR, acc16475_RSET); // Chip Select, Data Ready, Reset Pin Assignments

void setup() {
  Serial.begin(115200); // Initialize serial output via USB
  //IMU.configSPI(); // Configure SPI communication
  delay(500); // Give the part time to start up


  if (!SD.begin(SD_CS)) {
    Serial.println(F("Card failed, or not present"));
  }

  delay(500); // Give the part time to start up

  IMU.regWrite(MSC_CTRL, 0xC1);  // Enable Data Ready, set polarity
  //IMU.regWrite(FILT_CTRL, 0x04); // Set digital filter
  //IMU.regWrite(DEC_RATE, 0x03); // Disable decimation
  IMU.regWrite(FILT_CTRL, 0x00); // Set digital filter
  IMU.regWrite(DEC_RATE, 0x00); // Disable decimation
  //IMU.regWrite(NULL_CFG, 0x3F0A); //  bias estimation period  deafult 0x70A unenble 0xA all enble 3F0A
  //IMU.regWrite(GLOB_CMD, 0x01); // Bias Correction Update

  delay(30);

  logFileName = nextLogFile();
  Serial.println(F("done initialize"));

  delay(500);
  logFile = SD.open(logFileName, FILE_WRITE);
    if (logFile) {
    Serial.println(F("writing"));
    }
    // if the file didn't open, print an error:
    else {
    Serial.println(F("error opening file"));
    //return;
    }
  delay(1000);
}

void loop() {

  IMU.configSPI(); // Configure SPI communication
  if (digitalRead(acc16475_DR) == HIGH) {
    gxo = (IMU.regRead(X_GYRO_OUT) << 16) + IMU.regRead(X_GYRO_LOW);
    gyo = (IMU.regRead(Y_GYRO_OUT) << 16) + IMU.regRead(Y_GYRO_LOW);
    gzo = (IMU.regRead(Z_GYRO_OUT) << 16) + IMU.regRead(Z_GYRO_LOW);

    axo = (IMU.regRead(X_ACCL_OUT) << 16) + IMU.regRead(X_ACCL_LOW);
    ayo = (IMU.regRead(Y_ACCL_OUT) << 16) + IMU.regRead(Y_ACCL_LOW);
    azo = (IMU.regRead(Z_ACCL_OUT) << 16) + IMU.regRead(Z_ACCL_LOW);
  }

  gx = gxo;
  gy = gyo;
  gz = gzo;
  gx = gx / 10485760 ;
  gy = gy / 10485760 ;
  gz = gz / 10485760 ;

  ax = axo;
  ay = ayo;
  az = azo;
  ax = ax / 262144000;// * 9.80665;
  ay = ay / 262144000;// * 9.80665;
  az = az / 262144000;// * 9.80665;

  accdata = String(ax, 7) + ", " + String(ay, 7) + ", " + String(az, 7);
  gyrdata = String(gx, 6) + ", " + String(gy, 6) + ", " + String(gz, 6);
  logdata = accdata + "," + gyrdata;
  Serial.println(logdata);

  if (!logFile) {
    logFile = SD.open(logFileName, FILE_WRITE);
  }
  if (logFile) {
    //Serial.println("writing");
    logFile.println(logdata);
    logFile.close(); // close the file
  }
  // if the file didn't open, print an error:
  else {
    Serial.println("error opening test.txt");
  }
  //delayMicroseconds(2500); // Delay to not violate read rate
  delay(5); // Delay to not violate read rate
}

void getmydata() {
  gxo = (IMU.regRead(X_GYRO_OUT) << 16) + IMU.regRead(X_GYRO_LOW);
  gyo = (IMU.regRead(Y_GYRO_OUT) << 16) + IMU.regRead(Y_GYRO_LOW);
  gzo = (IMU.regRead(Z_GYRO_OUT) << 16) + IMU.regRead(Z_GYRO_LOW);

  axo = (IMU.regRead(X_ACCL_OUT) << 16) + IMU.regRead(X_ACCL_LOW);
  ayo = (IMU.regRead(Y_ACCL_OUT) << 16) + IMU.regRead(Y_ACCL_LOW);
  azo = (IMU.regRead(Z_ACCL_OUT) << 16) + IMU.regRead(Z_ACCL_LOW);

  gx = gxo;
  gx = gx / 10485760 ;
  gy = gyo;
  gy = gy / 10485760 ;
  gz = gzo;
  gz = gz / 10485760 ;

  ax = axo;
  ax = ax / 262144000 * 9.80665;
  ay = ayo;
  ay = ay / 262144000 * 9.80665;
  az = azo;
  az = az / 262144000 * 9.80665;
}

String nextLogFile(void) {
  String filename;
  int logn = 0;
  for (int i = 0; i < 999; i++) {
    // Construct a file with PREFIX[Index].SUFFIX
    filename = String("log") + String(logn) + String(".csv");
    // If the file name doesn't exist, return it
    if (!SD.exists(filename))
    {
      return filename;
    }
    // Otherwise increment the index, and try again
    logn++;
  }
  return "";
}
