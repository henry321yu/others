#include <SPI.h>
#include"register.h"
#include <SoftwareSerial.h>

SoftwareSerial HC12(7, 8);
String logFileName, accdata, gyrdata, magdata, logdata, mag2data, tmdata, halldata, flydata; // Rotordata;


const int mag1CS = 5;
const int mag2CS = 6;
int CS;
int ID, mn[2] = {5, 6}, delayy = 10000, setF = 100;
long values[10], t[12];
double x[3], y[3], z[3], xm[7], ym[7], zm[7], gx1, gy1, gz1, t0, timee = 0, f, f2, tm[7], tg; //, x1, y1, z1
int maxsize = 209715200;//209715200=200mb 262144000=250mb = 250*2^20;
unsigned long i = 0, t1, beepert;
const int H1 = A9, H2 = A8; //hall need wire
int setpin = 9;
double tt[10];
long ii = 500; long iii = 1000;

void setup() {
  Serial.begin(115200); // Initialize serial output via USB
  delay(100);
  Serial.println(F("Serial.begin"));
  Serial.println(F("SPI.begin"));
  SPI.begin();
  delay(100);

  Serial.println(F("HC12.begin"));
  HC12.begin(115200);
  delay(100);
  pinMode(setpin, OUTPUT);
  digitalWrite(setpin, LOW);
  delay(100);
  HC12.print("AT+B115200");
  delay(100);
  HC12.print("AT+C127");
  delay(100);
  HC12.print("AT+P8");
  delay(100);
  digitalWrite(setpin, HIGH);

  pinMode(H1, INPUT);
  pinMode(H2, INPUT);
  pinMode(mag1CS, OUTPUT);
  pinMode(mag2CS, OUTPUT);
  digitalWrite(mag1CS, HIGH);
  digitalWrite(mag2CS, HIGH);

  Serial.println(F("MMC5983MA setting"));
  //MMC5983MA n setting
  for (int j = 0; j < 2; j++) {
    CS = mn[j];
    swriteRegister(MMC5983MA_INTERNAL_CONTROL_0, 0x16); //reseting //2022/3/17 add back
    delayMicroseconds(150);

    swriteRegister(MMC5983MA_INTERNAL_CONTROL_1, 0x03); //2 200hz、350hz //3 600hz
    delayMicroseconds(150);
    swriteRegister(MMC5983MA_INTERNAL_CONTROL_0, 0x01); //initiate measurement
    delayMicroseconds(150);
  }

}

void loop() {
  // put your main code here, to run repeatedly:

  t0 = millis() * 0.001;

  mag_data_SPI();  //5 27
  magdata = String(timee, 3) + ", " +  String(xm[1], 5) + ", " + String(ym[1], 5) + ", " + String(zm[1], 5) + ", " + String(f2, 2) + ", " + String(f, 2) + ", " + String(delayy);
  Serial.println(magdata);

  HC12.println(magdata);


  //timer
  timer();
  delayMicroseconds(delayy);
}

void mag_data_SPI() {
  for (int j = 0; j < 2; j++) {
    CS = mn[j];
    swriteRegister(MMC5983MA_INTERNAL_CONTROL_0, 0x10); //get t,m data //only tm 0x02//set 0x10
    delayMicroseconds(30);
    tm[0] = sreadRegister(MMC5983MA_TOUT);
    delayMicroseconds(30);

    swriteRegister(MMC5983MA_INTERNAL_CONTROL_0, 0x01);
    delayMicroseconds(30);
    sreadmultiRegister (MMC5983MA_XOUT_0, 7);


    t[3] = values[0] << 10 | values[1] << 2 | bitRead(values[6], 7) << 1 | bitRead(values[6], 6);  //18bit
    t[4] = values[2] << 10 | values[3] << 2 | bitRead(values[6], 5) << 1 | bitRead(values[6], 4);
    t[5] = values[4] << 10 | values[5] << 2 | bitRead(values[6], 3) << 1 | bitRead(values[6], 2);

    xm[0] = t[3];
    ym[0] = t[4];
    zm[0] = t[5];

    xm[0] = (xm[0] - 0x1FFFF) / 163.84;  //18bit
    ym[0] = (ym[0] - 0x1FFFF) / 163.84;
    zm[0] = (zm[0] - 0x1FFFF) / 163.84;

    tm[0] -= 75;
    tm[0] = tm[0] * 0.7;

    xm[j + 1] = xm[0];
    ym[j + 1] = ym[0];
    zm[j + 1] = zm[0];
    switch (j) {
      case 0:
        tm[j + 1] = tm[0] - 16.4; break;
      case 1:
        tm[j + 1] = tm[0] - 15; break;//-18.5
    }
  }
}

void swriteRegister (byte thisRegister, byte value) { //spi communication
  digitalWrite(CS, LOW);
  SPI.transfer(thisRegister);
  SPI.transfer(value);
  digitalWrite(CS, HIGH);
}

byte sreadRegister (byte thisRegister) {
  byte inByte = 0 ;
  digitalWrite(CS, LOW);
  SPI.transfer(thisRegister | 0x80);
  inByte = SPI.transfer(0x00);
  digitalWrite(CS, HIGH);
  return inByte;
}

void sreadmultiRegister(byte thisRegister, int num) {
  int k = 0;
  memset(values, 0, sizeof(values));
  digitalWrite(CS, LOW);
  SPI.transfer(thisRegister | 0x80);
  while (k < num)
  {
    values[k++] = SPI.transfer(0x00);
  }
  digitalWrite(CS, HIGH);
}


void timer() {
  i++;
  timee = millis() * 0.001;
  f2 = i / timee;
  Serial.println(i);
  Serial.println(ii);
  Serial.println(iii);
  Serial.println(tt[3]);
  Serial.println(tt[1]);
  Serial.println(tt[2]);

  if (i >= iii) {
    iii = i + 500;
    tt[1] = timee;
    tt[3] = tt[0];
    tt[2] = tt[1] - tt[0];
    f = 500 / tt[2];
    if (abs(f - setF) > 1) {
      if (setF > f) {
        delayy -= (setF - f) * 100;
      }
      if (setF < f) {
        delayy += (f - setF) * 100;
      }
      if (delayy > 10000) {
        delayy = 10000;
      }
      if (delayy < 0) {
        delayy = 0;
      }
    }
  }

  if (i >= ii) {
    ii = i + 500;
    tt[0] = timee;
  }

}
