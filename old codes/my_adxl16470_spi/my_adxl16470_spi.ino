#include <SPI.h>
#include <ADIS16470.h>

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

ADIS16470 IMU(acc16475_CS, acc16475_DR, acc16475_RSET); // Chip Select, Data Ready, Reset Pin Assignments

void setup() {
  Serial.begin(115200); // Initialize serial output via USB
  IMU.configSPI(); // Configure SPI communication
  delay(500); // Give the part time to start up

  IMU.regWrite(MSC_CTRL, 0xC1);  // Enable Data Ready, set polarity
  //IMU.regWrite(FILT_CTRL, 0x04); // Set digital filter
  //IMU.regWrite(DEC_RATE, 0x03); // Disable decimation
  IMU.regWrite(FILT_CTRL, 0x00); // Set digital filter
  IMU.regWrite(DEC_RATE, 0x00); // Disable decimation
  //IMU.regWrite(NULL_CFG, 0x3F0A); //  bias estimation period  deafult 0x70A unenble 0xA all enble 3F0A
  //IMU.regWrite(GLOB_CMD, 0x01); // Bias Correction Update

  delay(500);
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

void loop() {
  //attachInterrupt(2, getmydata, RISING); // Attach interrupt to pin 2. Trigger on the rising edge

  if (digitalRead(acc16475_DR) == HIGH) {
    gxo = (IMU.regRead(X_GYRO_OUT) << 16) + IMU.regRead(X_GYRO_LOW);
    gyo = (IMU.regRead(Y_GYRO_OUT) << 16) + IMU.regRead(Y_GYRO_LOW);
    gzo = (IMU.regRead(Z_GYRO_OUT) << 16) + IMU.regRead(Z_GYRO_LOW);

    axo = (IMU.regRead(X_ACCL_OUT) << 16) + IMU.regRead(X_ACCL_LOW);
    ayo = (IMU.regRead(Y_ACCL_OUT) << 16) + IMU.regRead(Y_ACCL_LOW);
    azo = (IMU.regRead(Z_ACCL_OUT) << 16) + IMU.regRead(Z_ACCL_LOW);

    /*gxb = (IMU.regRead(XG_BIAS_HIGH) << 16) + IMU.regRead(XG_BIAS_LOW);
    gyb = (IMU.regRead(YG_BIAS_HIGH) << 16) + IMU.regRead(YG_BIAS_LOW);
    gzb = (IMU.regRead(ZG_BIAS_HIGH) << 16) + IMU.regRead(ZG_BIAS_LOW);

    axb = (IMU.regRead(XA_BIAS_HIGH) << 16) + IMU.regRead(XA_BIAS_LOW);
    ayb = (IMU.regRead(YA_BIAS_HIGH) << 16) + IMU.regRead(YA_BIAS_LOW);
    azb = (IMU.regRead(ZA_BIAS_HIGH) << 16) + IMU.regRead(ZA_BIAS_LOW);*/
  }

  //IMU.regWrite(GLOB_CMD, 0x01); // Bias Correction Update


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

  Serial.print(gx, 6);
  Serial.print("\t");
  Serial.print(gy, 6);
  Serial.print("\t");
  Serial.print(gz, 6);
  Serial.print("\t");
  Serial.print(ax, 6);
  Serial.print("\t");
  Serial.print(ay, 6);
  Serial.print("\t");
  Serial.print(az, 6);
  Serial.print("\t");

  /*Serial.print(gxb);
  Serial.print("\t");
  Serial.print(gyb);
  Serial.print("\t");
  Serial.print(gzb);
  Serial.print("\t");
  Serial.print(axb);
  Serial.print("\t");
  Serial.print(ayb);
  Serial.print("\t");
  Serial.print(azb);
  Serial.print("\t");*/



  Serial.print("\n");
  //delayMicroseconds(2500); // Delay to not violate read rate
  //delayMicroseconds(2500); // Delay to not violate read rate
  delay(5); // Delay to not violate read rate
  //while (1);
}
