#define ACCEL_XOUT_H  0x3B //MPU9250
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

#include <WSWire.h>

const int IMU_I2c = 0x68; //
const int led = 4; //
long values[20], t[9];
double x, y, z, x1, y1, z1, c;


void setup() {
  Serial.begin(115200);
  Wire.begin();
  //TWBR = 12;

  //writeRegister(PWR_MGMT_1, 0x80); //reset
  writeRegister(PWR_MGMT_2, 0x00);
  writeRegister(PWR_MGMT_1, 0x01); //enable all axis
  //writeRegister(ACCEL_CONFIG, 0xE0); // Accel self-test
  //writeRegister(ASTC, 0x01);
  Serial.println("done initialize");


  delay(200);
}
void loop() {

  imu_data();

  Serial.print(x1, 5);
  Serial.print(", ");
  Serial.print(y1, 5);
  Serial.print(", ");
  Serial.println(z1, 5);

  delay(1000);
}

void writeRegister(int reg, int data) {
  Wire.beginTransmission(IMU_I2c);
  Wire.write(reg);
  Wire.write(data);
  Wire.endTransmission();
}

int readRegister(int reg) {
  Wire.beginTransmission(IMU_I2c);
  Wire.write(reg);
  Wire.endTransmission();
  Wire.requestFrom(IMU_I2c, 1);
  if (Wire.available() <= 1) {
    return Wire.read();
  }
}


void imu_data() {
  values[10] = readRegister(ACCEL_XOUT_H);
  values[11] = readRegister(ACCEL_XOUT_L);
  values[12] = readRegister(ACCEL_YOUT_H);
  values[13] = readRegister(ACCEL_YOUT_L);
  values[14] = readRegister(ACCEL_ZOUT_H);
  values[15] = readRegister(ACCEL_ZOUT_L);

  x1 = (values[10] << 8) + values[11];
  y1 = (values[12] << 8) + values[13];
  z1 = (values[14] << 8) + values[15];

  if (x1 >= 0x8000) {
    x1 = x1 -  0x10000;
  }
  if (y1 >= 0x8000) {
    y1 = y1 -  0x10000;
  }
  if (z1 >= 0x8000) {
    z1 = z1 -  0x10000;
  }

  x1 = x1/0x4000;
  y1 = y1/0x4000;
  z1 = z1/0x4000;
}
