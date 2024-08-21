#include <Wire.h>
#include"register.h"

const int mag_I2c = 0x30; //
int ID;
long values[60], t[10];
double x, y, z, c, xm1, ym1, zm1, gx1, gy1, gz1, xm2, ym2, zm2, time, f;//, x1, y1, z1
String logFileName, accdata, grydata, magdata, logdata, mag2data;
unsigned long i = 0, t0;


void setup() {
  Serial.begin(115200);
  Wire.begin();
  Wire.setClock(400000);
  //TWBR = 12;

  //MMC5883MA 1 setting
  writeRegister(mag_I2c, MMC5883MA_INTERNAL_CONTROL_0, 0x08);
  delay(30);


  writeRegister(mag_I2c , MMC5883MA_INTERNAL_CONTROL_0, 0x01); //initiate measurement
  delay(30);
  //writeRegister(mag_I2c ,MMC5883MA_STATUS, 0x0);
  writeRegister(mag_I2c , MMC5883MA_INTERNAL_CONTROL_1, 0x02);
  delay(30);

  Serial.println(F("done initialize"));

  delay(1000);
  t0 = millis();
}
void loop() {
  mag_data();
  magdata = String(xm1, 5) + ", " + String(ym1, 5) + ", " + String(zm1, 5);
  Serial.println(magdata);
  timer();
  delayMicroseconds(2500);//100hz 5400 200hz 440
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


void mag_data() {
  writeRegister(mag_I2c, MMC5883MA_INTERNAL_CONTROL_0, 0x01);
  ID = mag_I2c;
  values[20] = readRegister(MMC5883MA_XOUT_LOW);
  values[21] = readRegister(MMC5883MA_XOUT_HIGH);
  values[22] = readRegister(MMC5883MA_YOUT_LOW);
  values[23] = readRegister(MMC5883MA_YOUT_HIGH);
  values[24] = readRegister(MMC5883MA_ZOUT_LOW);
  values[25] = readRegister(MMC5883MA_ZOUT_HIGH);

  xm1 = values[21] << 8 | values[20];
  ym1 = values[23] << 8 | values[22];
  zm1 = values[25] << 8 | values[24];

  xm1 -= 0x8000;
  ym1 -= 0x8000;
  zm1 -= 0x8000;

  xm1 = xm1 / 40.96;
  ym1 = ym1 / 40.96;
  zm1 = zm1 / 40.96;
}

void timer() {
  time = (millis() - t0) * 0.001;
  //Serial.print(time,3);
  //Serial.print("    i= ");
  //Serial.println(i);
  i++;
  if (i % 250 == 0) {
    writeRegister(mag_I2c, MMC5883MA_INTERNAL_CONTROL_0, 0x08);
    if (i % 1000 == 0) {
      f = i / time;
      //Serial.print(F("freqency= ")); Serial.print(f); Serial.println(F(" Hz"));
      //i = 0;
    }
  }
}
