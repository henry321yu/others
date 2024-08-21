#define MMC5883MA_XOUT_LOW 0x00
#define MMC5883MA_XOUT_HIGH 0x01
#define MMC5883MA_YOUT_LOW 0x02
#define MMC5883MA_YOUT_HIGH 0x03
#define MMC5883MA_ZOUT_LOW 0x04
#define MMC5883MA_ZOUT_HIGH 0x05
#define MMC5883MA_TEMPERATURE 0x06
#define MMC5883MA_STATUS 0x07
#define MMC5883MA_INTERNAL_CONTROL_0 0x08
#define MMC5883MA_INTERNAL_CONTROL_1 0x09
#define MMC5883MA_INTERNAL_CONTROL_2 0x0A
#define MMC5883MA_X_THRESHOLD 0x0B
#define MMC5883MA_Y_THRESHOLD 0x0C
#define MMC5883MA_Z_THRESHOLD 0x0D
#define MMC5883MA_PRODUCT_ID 0x2F

#include <Wire.h>
#define TCAADDR 0x70

void tcaselect(uint8_t i) {
  if (i > 7) return;

  Wire.beginTransmission(TCAADDR);
  Wire.write(1 << i);
  Wire.endTransmission();
}


const int mag_I2c = 0x30; //
const int led = 4; //
long values[40], t[9], mt[10];
double x, y, z, c, xm1, ym1, zm1, tm1;


void setup() {
  Serial.begin(115200);
  Wire.begin();
  Wire.setClock(400000);
  //TWBR = 12;
  tcaselect(2);

  writeRegister(MMC5883MA_INTERNAL_CONTROL_1, 0x80);
  delay(30);
  writeRegister(MMC5883MA_INTERNAL_CONTROL_0, 0x08);
  delay(30);
  writeRegister(MMC5883MA_INTERNAL_CONTROL_0, 0x04);
  delay(30);

  writeRegister(MMC5883MA_INTERNAL_CONTROL_0, 0x01); //initiate measurement
  delay(30);
  writeRegister(MMC5883MA_INTERNAL_CONTROL_1, 0x02);

  Serial.println("done initialize");


  delay(200);
}
void loop() {
  mag_data();


  Serial.print(xm1, 5);
  Serial.print(", ");
  Serial.print(ym1, 5);
  Serial.print(", ");
  Serial.print(zm1, 5);
  Serial.print(", ");
  Serial.println(tm1, 2);

  delay(10);
}

void writeRegister(int reg, int data) {
  Wire.beginTransmission(mag_I2c);
  Wire.write(reg);
  Wire.write(data);
  Wire.endTransmission();
}

int readRegister(int reg) {
  Wire.beginTransmission(mag_I2c);
  Wire.write(reg);
  Wire.endTransmission();
  Wire.requestFrom(mag_I2c, 1);
  if (Wire.available() <= 1) {
    return Wire.read();
  }
}
void readmultiRegister(int fst,int num) {
  int k=0;
  Wire.beginTransmission(mag_I2c);
  Wire.write(fst);
  Wire.endTransmission();
  Wire.requestFrom(mag_I2c, num);
  while(Wire.available() && k<num)
  {
    values[k++]=Wire.read();
  }
  }


void mag_data() {
  tcaselect(2);

  writeRegister(MMC5883MA_INTERNAL_CONTROL_0, 0x02);  //temperature reading
  values[26] = readRegister(MMC5883MA_TEMPERATURE);
  tm1 = values[26];

  writeRegister(MMC5883MA_INTERNAL_CONTROL_0, 0x09);
  delay(1);

  readmultiRegister(MMC5883MA_XOUT_LOW,6);
  /*values[20] = readRegister(MMC5883MA_XOUT_LOW);
  values[21] = readRegister(MMC5883MA_XOUT_HIGH);
  values[22] = readRegister(MMC5883MA_YOUT_LOW);
  values[23] = readRegister(MMC5883MA_YOUT_HIGH);
  values[24] = readRegister(MMC5883MA_ZOUT_LOW);
  values[25] = readRegister(MMC5883MA_ZOUT_HIGH);*/

  mt[0] = values[1] << 8 | values[0];
  mt[1] = values[3] << 8 | values[2];
  mt[2] = values[5] << 8 | values[4];

  writeRegister(MMC5883MA_INTERNAL_CONTROL_0, 0x05);
  delay(1);
  
  readmultiRegister(MMC5883MA_XOUT_LOW,6);
  /*values[20] = readRegister(MMC5883MA_XOUT_LOW);
  values[21] = readRegister(MMC5883MA_XOUT_HIGH);
  values[22] = readRegister(MMC5883MA_YOUT_LOW);
  values[23] = readRegister(MMC5883MA_YOUT_HIGH);
  values[24] = readRegister(MMC5883MA_ZOUT_LOW);
  values[25] = readRegister(MMC5883MA_ZOUT_HIGH);*/

  mt[3] = values[1] << 8 | values[0];
  mt[4] = values[3] << 8 | values[2];
  mt[5] = values[5] << 8 | values[4];

  xm1 = (mt[0] + mt[3]) / 2;
  ym1 = (mt[1] + mt[4]) / 2;
  zm1 = (mt[2] + mt[5]) / 2;

  /*xm1 = values[21] << 8 | values[20];
    ym1 = values[23] << 8 | values[22];
    zm1 = values[25] << 8 | values[24];*/

  xm1 -= 0x8000;
  ym1 -= 0x8000;
  zm1 -= 0x8000;

  xm1 = xm1 / 40.96;
  ym1 = ym1 / 40.96;
  zm1 = zm1 / 40.96;

  tm1 -= 75;
  tm1 = tm1 * 0.7-16.4;
}





/*#include <Wire.h>

  #define MMC5883MA_OUT 0x00
  #define MMC5883MA_XOUT 0x00
  #define MMC5883MA_XOUT_LOW 0x00
  #define MMC5883MA_XOUT_HIGH 0x01
  #define MMC5883MA_YOUT 0x02
  #define MMC5883MA_YOUT_LOW 0x02
  #define MMC5883MA_YOUT_HIGH 0x03
  #define MMC5883MA_ZOUT 0x04
  #define MMC5883MA_ZOUT_LOW 0x04
  #define MMC5883MA_ZOUT_HIGH 0x05
  #define MMC5883MA_TEMPERATURE 0x06
  #define MMC5883MA_STATUS 0x07
  #define MMC5883MA_INTERNAL_CONTROL_0 0x08
  #define MMC5883MA_INTERNAL_CONTROL_1 0x09
  #define MMC5883MA_INTERNAL_CONTROL_2 0x0A
  #define MMC5883MA_X_THRESHOLD 0x0B
  #define MMC5883MA_Y_THRESHOLD 0x0C
  #define MMC5883MA_Z_THRESHOLD 0x0D
  #define MMC5883MA_PRODUCT_ID 0x2F
  #define MMC5883MA_ADDR 0x30

  #define MMC5883MA_DYNAMIC_RANGE 16
  #define MMC5883MA_RESOLUTION 65536

  uint8_t id;
  uint8_t iStatus;
  uint8_t temp;
  uint8_t result[6];

  void setup() {

  Wire.begin();
  Serial.begin(9600);
  delay(100);

  //// SET
  Wire.beginTransmission(MMC5883MA_ADDR); //Adress of I2C device
  Wire.write(MMC5883MA_INTERNAL_CONTROL_0);// I2C Register
  Wire.write(0x08);// Value
  Wire.endTransmission();

  // SET A TEMPERATURE MEASUREMENT
  Wire.beginTransmission(MMC5883MA_ADDR); //Adress of I2C device
  Wire.write(MMC5883MA_INTERNAL_CONTROL_0);// I2C Register
  Wire.write(0x02);// Value
  Wire.endTransmission();
  }

  void loop() {
  Wire.beginTransmission(MMC5883MA_ADDR); //Adress of I2C device
  Wire.write(MMC5883MA_STATUS);// I2C Status Register
  Wire.requestFrom(MMC5883MA_ADDR, 1);
  iStatus = Wire.read(); //Status here is 127
  Wire.endTransmission(false);

  Wire.beginTransmission(MMC5883MA_ADDR); //Adress of I2C device
  Wire.write(MMC5883MA_TEMPERATURE);// I2C Register
  Wire.requestFrom(MMC5883MA_ADDR, 1);
  temp = Wire.read(); //TEMP value is 18 (-62°C...)
  Wire.endTransmission(false);

  Serial.println(temp);
  }*/
