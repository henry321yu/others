#define DEVID_AD                 0x00 //ADXL355BZ
#define DEVID_MST                0x01
#define PARTID                   0x02
#define REVID                    0x03
#define STATUS                   0x04
#define FIFO_ENTRIES             0x05
#define TEMP2                    0x06
#define TEMP1                    0x07
#define XDATA3                   0x08
#define XDATA2                   0x09
#define XDATA1                   0x0A
#define YDATA3                   0x0B
#define YDATA2                   0x0C
#define YDATA1                   0x0D
#define ZDATA3                   0x0E
#define ZDATA2                   0x0F
#define ZDATA1                   0x10
#define FIFO_DATA                0x11
#define OFFSET_X_H               0x1E
#define OFFSET_X_L               0x1F
#define OFFSET_Y_H               0x20
#define OFFSET_Y_L               0x21
#define OFFSET_Z_H               0x22
#define OFFSET_Z_L               0x23
#define ACT_EN                   0x24
#define ACT_THRESH_H             0x25
#define ACT_THRESH_L             0x26
#define ACT_COUNT                0x27
#define FILTER                   0x28
#define FIFO_SAMPLES             0x29
#define INT_MAP                  0x2A
#define SYNC                     0x2B
#define RANGE                    0x2C
#define POWER_CTL                0x2D
#define SELF_TEST                0x2E
#define RESET                    0x2F

#include <SD.h>
#include <SPI.h>
#include <Wire.h>

const int SD_CS = 10; // Pin 10 on Arduino Uno
const int acc_I2c = 0x1D; //
const int led = 4; //
int ID, rd, rdd, delayy = 0, setF=100;
long values[10], t[9];
double x, y, z, at, time, f;
String logFileName, accdata, logdata;
File logFile;
unsigned long i = 0, t0;


void setup() {
  Serial.begin(115200);
  Wire.begin();
  Wire.setClock(100000);
  //TWBR = 12;

  if (!SD.begin(SD_CS)) {
    Serial.println(F("Card failed, or not present"));
  }
  pinMode(SD_CS, OUTPUT);
  logFileName = nextLogFile();

  //adxl355 setting
  ID = acc_I2c;
  writeRegister(acc_I2c, RESET, 0x52);  // reset sensor
  delay(100);
  writeRegister(acc_I2c, POWER_CTL, 0x00);  // writing 0 to to enable sensor
  delay(30);
  writeRegister(acc_I2c, RANGE, 0x02);
  delay(30);
  writeRegister(acc_I2c, SELF_TEST, 0x00);  // writing 3 to to enable self test
  delay(100);

  /*rd = readRegister(STATUS);
    rdd=rd;
    while(1){
    rd = readRegister(STATUS);
    Serial.println(rd,BIN);
    if(rd!=rdd){
      break;
      }
    rdd=rd;
    }*/



  logFile = SD.open(logFileName, FILE_WRITE);
  if (logFile) {
    Serial.println(F("writing"));
    //test0.print("當您取消產品保修時。然後你真正擁有了這個產品。");
    //test0.close(); // close the file
  }
  // if the file didn't open, print an error:
  else {
    Serial.println(F("error opening file"));
    //return;
  }
  pinMode(led, OUTPUT);
  digitalWrite(led, LOW);

  delay(200);
  t0 = millis();
}
void loop() {
  acc_data();

  accdata = String(x, 5) + ", " + String(y, 5) + ", " + String(z, 5);
  logdata = String(time, 3) + ", " + accdata;
  Serial.println(logdata);

  //delay(1);
  if (!logFile) {
    logFile = SD.open(logFileName, FILE_WRITE);
    //Serial.println(F("file opened"));
  }
  //delay(1);
  if (logFile) {
    //Serial.println(F("writing"));
    digitalWrite(led, HIGH);
    logFile.println(logdata);

    //logFile.close(); // close the file
  }
  else {
    Serial.println(F("error opening test.txt"));
    //return;
  }
  delayMicroseconds(5650 + delayy);
  digitalWrite(led, LOW);
  timer();
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
  readmultiRegister(XDATA3, 9);

  x = values[0] << 12 | values[1] << 4 | values[2] >> 4;
  y = values[3] << 12 | values[4] << 4 | values[5] >> 4;
  z = values[6] << 12 | values[7] << 4 | values[8] >> 4;

  if (x >= 0x80000)
    x = x - (2 * 0x80000);
  if (y >= 0x80000)
    y = y - (2 * 0x80000);
  if (z >= 0x80000)
    z = z - (2 * 0x80000);

  t[8] = readRegister(TEMP2);
  t[9] = readRegister(TEMP1);

  at = (t[8] << 8) + t[9];
  at = ((1852 - at) / 9.05) + 27.2;

  //x = x / 256000;
  //y = y / 256000;
  //z = z / 256000;
  x = x / 128000;
  y = y / 128000;
  z = z / 128000;
}

String nextLogFile(void)
{
  String filename;
  int logn = 0;

  for (int i = 0; i < 999; i++) {
    // Construct a file with PREFIX[Index].SUFFIX
    filename = String("log") + String(logn) + String(".txt");
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
void timer() {
  time = (millis() - t0) * 0.001;
  f = i / time;
  if (i % 100 == 0) {
    logFile.close(); // close the file
    if (i % 1000 == 0) {
      Serial.print(F("freqency= ")); Serial.print(f); Serial.println(F(" Hz"));
    }
  }
  if (time < 0x30000) {
    if (f < setF) {
      delayy -= 10;
    }
    if (f > setF) {
      delayy += 10;
    }
    //Serial.println(delayy);
    i++;
  }
}
