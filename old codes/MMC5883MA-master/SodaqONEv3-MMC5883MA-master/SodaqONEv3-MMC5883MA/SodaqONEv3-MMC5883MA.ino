#include <Wire.h>
#include <TheThingsNetwork.h>

#define debugSerial SerialUSB
#define loraSerial Serial1

//*****************************************************
// The things network
//*****************************************************
// Set your AppEUI and AppKey
const char *appEui = "***";
const char *appKey = "***";

#define freqPlan TTN_FP_US915 // assign your freq. zone
TheThingsNetwork ttn(loraSerial, debugSerial, freqPlan);

//*****************************************************
// MMC5883MA Register map
//*****************************************************
#define XOUT_LSB    0x00
#define XOUT_MSB    0x01
#define YOUT_LSB    0x02
#define YOUT_MSB    0x03
#define ZOUT_LSB    0x04
#define ZOUT_MSB    0x05
#define TEMPERATURE 0x06
#define STATUS      0x07
#define INT_CTRL0   0x08
#define INT_CTRL1   0x09
#define INT_CTRL2   0x0A
#define X_THRESHOLD 0x0B
#define Y_THRESHOLD 0x0C
#define Z_THRESHOLD 0x0D
#define PROD_ID1    0x2F

#define MMC5883MA   0x30                          // Sensor I2C address

#define MMC5883MA_DYNAMIC_RANGE 16
#define MMC5883MA_RESOLUTION    65536

//*****************************************************
// Global variables declaration
//*****************************************************


//*****************************************************
// Functions declaration
//*****************************************************
char read_register(byte REG_ADDR);
void write_register(byte REG_ADDR, byte VALUE);
void wait_meas(void);
void reset_sensor(void);
float parser(char axis_msb, char axis_lsb);

void message(const uint8_t *payload, size_t size, port_t port);

//*****************************************************
// Setup
//*****************************************************
void setup() 
{
  Wire.begin();                                   // Join I2C bus (address optional for master)
  debugSerial.begin(9600);                        // Start USB serial port
  loraSerial.begin(57600);                        // Start LoRa module serial port
  
  //----------------------------------  
  while (!debugSerial && millis() < 10000);       // Wait a maximum of 10s for Serial Monitor
  debugSerial.println("-- STATUS");               // Status request
  ttn.showStatus();
  debugSerial.println("-- JOIN");                 // Join request
  ttn.join(appEui, appKey);
  ttn.onMessage(message);                         // Handle downlink data
  //----------------------------------  
  
  write_register(INT_CTRL0, 0x04);                // SET instruction
}

//*****************************************************
// Main
//*****************************************************
void loop()
{
  debugSerial.println("---------");
  
  //----------------------------------
  // Variables initialization
  
  char x_lsb, x_msb, y_lsb, y_msb, z_lsb, z_msb;
  float x_val, y_val, z_val;
  uint8_t status_reg = 0;
  uint8_t id = 0;
  uint8_t payload[6] = {};
  //----------------------------------
  
  reset_sensor();
  
  write_register(INT_CTRL2, 0x40);                // Enables measurement interrupt
  write_register(STATUS, 0x01);                   // Clean measurement interrupt

  
  // Check status register before start magnetic field measurement
  status_reg = read_register(STATUS);
  //debugSerial.print("Status register before: ");
  //debugSerial.println(status_reg, BIN);
  
  //debugSerial.println("Starting measurement");
  write_register(INT_CTRL0, 0X01);                // Start magnetic field measurement

  wait_meas();
  
  // Check status register after complete magnetif field measurement
  status_reg = read_register(STATUS);
  //debugSerial.print("Status register after: ");
  //debugSerial.println(status_reg, BIN);
  
  x_lsb = read_register(XOUT_LSB);                // Read magnetic field - x lsb
  x_msb = read_register(XOUT_MSB);                // Read magnetic field - x msb
  y_lsb = read_register(YOUT_LSB);                // Read magnetic field - y lsb
  y_msb = read_register(YOUT_MSB);                // Read magnetic field - y msb
  z_lsb = read_register(ZOUT_LSB);                // Read magnetic field - z lsb
  z_msb = read_register(ZOUT_MSB);                // Read magnetic field - z msb

  x_val = parser(x_msb, x_lsb);
  y_val = parser(y_msb, y_lsb);
  z_val = parser(z_msb, z_lsb);

  debugSerial.print("Xout: ");
  debugSerial.println(x_val, 7);
  debugSerial.print("Yout: ");
  debugSerial.println(y_val, 7);
  debugSerial.print("Zout: ");
  debugSerial.println(z_val, 7);

  //----------------------------------
  // TTN payload configuration
  payload[0] = x_lsb;
  payload[1] = x_msb;
  payload[2] = y_lsb;
  payload[3] = y_msb;
  payload[4] = z_lsb;
  payload[5] = z_msb;
  ttn.sendBytes(payload, sizeof(payload));
  //----------------------------------
  
  delay(1000);
}

//*****************************************************
// Functions definition
//*****************************************************
char read_register(byte REG_ADDR)
{ 
  char reg_value = 0;

  Wire.beginTransmission(byte(MMC5883MA));        // Adress of I2C device
  Wire.write(byte(REG_ADDR));                     // Register address
  Wire.endTransmission();
  
  Wire.requestFrom(byte(MMC5883MA), 1);           // Request 1 byte from I2C slave device
  if (Wire.available() == 1)
  {
    reg_value = Wire.read();                      // Receive a byte as character
  } 
  Wire.endTransmission();
  return reg_value;
}
//-------------------------------------------------
void write_register(byte REG_ADDR, byte VALUE)
{
  Wire.beginTransmission(byte(MMC5883MA));        // Adress of I2C device
  Wire.write(byte(REG_ADDR));                     // Register address
  Wire.write(byte(VALUE));                        // Value to be written
  Wire.endTransmission();
}
//-------------------------------------------------
void reset_sensor()
{
  write_register(INT_CTRL1, 0x80);
  //Serial.println("Sensor reseted");
}
//-------------------------------------------------
void wait_meas()
{
  //Serial.println("Waiting for measurement");
  uint8_t status_reg = 0;
  uint8_t meas_finish = 0;
  byte mask = 1;

  while(meas_finish == 0)
  {
    status_reg = read_register(STATUS);
    meas_finish = status_reg & mask;
  }
  //Serial.println("Measurement finished");
}
//-------------------------------------------------
float parser(char MSB, char LSB)
{
  float ans = (float)(MSB << 8 | LSB) * MMC5883MA_DYNAMIC_RANGE / MMC5883MA_RESOLUTION - (float)MMC5883MA_DYNAMIC_RANGE / 2;
  return ans;
}
//-------------------------------------------------
void message(const uint8_t *payload, size_t size, port_t port)
{
  debugSerial.println("-- MESSAGE");
  debugSerial.print("Received " + String(size) + " bytes on port " + String(port) + ":");
  debugSerial.println();
}
