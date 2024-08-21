#include <MPU9250_RegisterMap.h>
#include <SparkFunMPU9250-DMP.h>



#include <SPI.h>


#define ACCEL_XOUT_H  0x3B
#define ACCEL_XOUT_L  0x3C
#define ACCEL_YOUT_H  0x3D
#define ACCEL_YOUT_L  0x3E
#define ACCEL_ZOUT_H  0x3F
#define ACCEL_ZOUT_L  0x40
#define WHO_AM_I      0x75
#define PWR_MGMT_1    0x6B
#define PWR_MGMT_2    0x6C
#define ST_X_ACCEL    0x0D
#define ST_Y_ACCEL    0x0E
#define ST_Z_ACCEL    0x0F
#define ACCEL_CONFIG  0x1C
#define ACCEL_CONFIG2 0x1D

MPU9250 IMU（Wire,0x68）;
const int chipSelectPin = 10;
int values[20],t[9];
double x, y, z,x0,y0,z0;

void setup() {
  Serial.begin(115200);

  SPI.begin();
  SPI.setClockDivider(SPI_CLOCK_DIV16);
  SPI.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE3));

  pinMode(chipSelectPin, OUTPUT);
  
  //writeRegister(PWR_MGMT_1, 0x80); //reset
  writeRegister(PWR_MGMT_2, 0x00);
  writeRegister(PWR_MGMT_1, 0x01); //enable all axis
  writeRegister(ACCEL_CONFIG, 0xE0); // Accel self-test
  //writeRegister(ASTC, 0x01);
  
  digitalWrite(chipSelectPin, HIGH);

  delay(100);
}
void loop() {

  values[0] = readRegister(ACCEL_XOUT_H);
  values[1] = readRegister(ACCEL_XOUT_L);
  values[2] = readRegister(ACCEL_YOUT_H);
  values[3] = readRegister(ACCEL_YOUT_L);
  values[4] = readRegister(ACCEL_ZOUT_H);
  values[5] = readRegister(ACCEL_ZOUT_L);

  t[0]=readRegister(WHO_AM_I);
  t[1]=readRegister(ST_X_ACCEL);
  t[2]=readRegister(ST_Y_ACCEL);
  t[3]=readRegister(ST_Z_ACCEL);
  
  x= (values[0] << 8) + values[1];
  y= (values[2] << 8) + values[3];
  z= (values[4] << 8) + values[5];
  
  Serial.print(x);
  Serial.print(',');
  Serial.print(y);
  Serial.print(',');
  Serial.print(z);
  Serial.print(',');
  Serial.print(t[0]);
  Serial.print(',');
  Serial.print(t[1]);
  Serial.print(',');
  Serial.print(t[2]);
  Serial.print(',');
  Serial.print(t[3]);
  Serial.print('\n');
  delay(5);
}

byte readRegister (byte thisRegister) {
  byte inByte = 0 ;
  digitalWrite(chipSelectPin, LOW);
  SPI.transfer(thisRegister | 0x80);
  inByte = SPI.transfer(0x00);
  digitalWrite(chipSelectPin, HIGH);
  return inByte;
}
void writeRegister (byte thisRegister, byte value) {
  digitalWrite(chipSelectPin, LOW);
  SPI.transfer(thisRegister);
  SPI.transfer(value);
  digitalWrite(chipSelectPin, HIGH);
}
