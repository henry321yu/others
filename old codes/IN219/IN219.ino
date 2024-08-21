#include <Wire.h>
#include <SD.h>
#include <SPI.h>
// small board change:sd,led,beeper,hall,mpu6050 set&read tcaselect(2)
const int v_I2c = 0x40; //
const int MPU_addr = 0x68;
const int SD_CS = 10; // teensy builtin
const int beeper = 4;
const int mRD = 21;
int ID, delayy = 0, setF = 100;
long values[20], t[12];
double x[3], y[3], z[3], xm[7], ym[7], zm[7], t0, ttime = 0, f, tm[7], ta[3], tg; //, x1, y1, z1
String accdata, gyrdata, magdata, logdata, mag2data, tmdata, halldata;            // Rotordata;
unsigned long i = 0, t1, beepert;
int maxsize = 262144000; //250mb = 250*2^20;
File logFile;
String fileName;
char logFileName[11];
int Addr;
int wireread = A3;

void writeRegister(int ID, int reg, int data)
{
  Wire.beginTransmission(ID);
  Wire.write(reg);
  Wire.write(data);
  Wire.endTransmission();
}

int readRegister(int reg)
{
  Wire.beginTransmission(ID);
  Wire.write(reg);
  Wire.endTransmission();
  Wire.requestFrom(ID, 1);
  if (Wire.available() <= 1)
  {
    return Wire.read();
  }
}

void readmultiRegister(int fst, int num)
{
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

void wireV()
{
  ID = v_I2c;

  readmultiRegister(0x02, 2);

  int16_t xmm = values[0] << 8 | values[1];

  xm[0] = xmm;
  //xm[1] = xm[0]/2048;
  xm[1] = (xmm >> 3) * 4 * 0.001;
  xm[2] = xm[1] * 9.023;

  readmultiRegister(0x01, 2);

  int16_t ymm = values[0] << 8 | values[1];

  ym[0] = ymm;
  //ym[1] = ym[0]/2048;
  ym[1] = ym[0] * 0.01;
  ym[2] = ym[1] * 9.023;

  //readmultiRegister(0x05, 2);

  //int16_t zmm = values[0] << 8 | values[1];

  zm[0] = ym[1] / 0.1;
}

void setup()
{
  Serial.begin(115200);
  Wire.begin();
  Wire.setClock(400000);
  //Wire.setClock(100000);

  Serial.println(F("done initialize"));
}

void loop()
{
  wireV();
  accdata = String(xm[2], 5) + ", " + String(ym[2], 5) + ", " + String(xm[1], 5) + ", " + String(ym[1], 5) + ", " + String(zm[0], 5);
  Serial.println(accdata);
  delay(10);
}
