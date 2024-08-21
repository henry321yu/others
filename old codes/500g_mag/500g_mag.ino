#include <SPI.h>
#include"register.h"

#define CS 10
#define EN 7
#define TRIGG 4
#define LASER 5
#define INTB1 8
#define FILT_SIZE 64
#define TDC_CLK 8 //8 MHZ
#define INTled 3
#define TRIGGled 2

uint8_t trig_ready = 0;
uint8_t donee = 0;
uint8_t state = 0;
double last[FILT_SIZE];
uint8_t i = 0;
double mean;
int start1pin = 19;
int stop1pin = 18;
uint32_t j = 0;
double timee[10];
int sipin = 17;

int32_t read3(uint8_t reg_add)
{
  int _CS = CS;
  // Write register address to be read
  digitalWrite(_CS, LOW); // Set CS low to enable device
  //SPI.transfer(reg_add); // Write address over SPI bus
  SPI.transfer(1 << 7 | 0 << 6 | reg_add); // Write address over SPI bus
  int32_t readd = SPI.transfer(0x00) << 16; // Send (0x00) and place upper byte into variable
  readd |= SPI.transfer(0x00) << 8; // Send (0x00) and place lower byte into variable
  readd |= SPI.transfer(0x00); // Send (0x00) and place lower byte into variable
  digitalWrite(_CS, HIGH); // Set CS high to disable device

  return readd;
}

uint8_t read1(uint8_t reg_add)
{
  int _CS = CS;
  // Write register address to be read
  digitalWrite(_CS, LOW); // Set CS low to enable device
  //SPI.transfer(reg_add); // Write address over SPI bus
  SPI.transfer(0 << 7 | 0 << 6 | reg_add); // Write address over SPI bus
  uint8_t readd = SPI.transfer(0x00); // Send (0x00) and place upper byte into variable
  digitalWrite(_CS, HIGH); // Set CS high to disable device

  return readd;
  //Serial.println(1 << 14 | reg_add << 8 | dataa, BIN);
}

void writeRegister(uint8_t reg_add, uint8_t dataa)
{
  int _CS = CS;
  digitalWrite(_CS, LOW); // Set CS low to enable device
  
  SPI.transfer(1 << 14 | reg_add << 8 | dataa); // Write address over SPI bus
  
  //SPI.transfer(1 << 6 | reg_add); // Write address over SPI bus
  //SPI.transfer(dataa); // Write address over SPI bus
  
  digitalWrite(_CS, HIGH); // Set CS high to disable device
  //Serial.println(1 << 14 | reg_add << 8 | dataa, BIN);
}

void setup() {
  // initialize digital pin LED_BUILTIN as an output.
  SPI.begin();
  SPI.beginTransaction(SPISettings (10000000, MSBFIRST, SPI_MODE3));
  //SPI.setClockDivider(84); //??(due only)

  //SPI.setClockDivider(SPI_CLOCK_DIV8); //??(due only)

  //pinMode(LED_BUILTIN, OUTPUT);
  pinMode(LASER, OUTPUT);
  pinMode(EN, OUTPUT);
  pinMode(start1pin, OUTPUT);
  pinMode(stop1pin, INPUT);
  pinMode(TRIGGled, OUTPUT);
  pinMode(INTled, OUTPUT);

  digitalWrite(LASER, HIGH);

  digitalWrite(EN, LOW);
  delay(100);
  digitalWrite(EN, HIGH);
  delay(100);

  digitalWrite(start1pin, LOW);
  digitalWrite(sipin, LOW);

  Serial.begin(115200);
  Serial.println("initialize over");
  delay(2000);


  pinMode(TRIGG, INPUT_PULLUP);
  attachInterrupt(TRIGG, triggerr, RISING);

  pinMode(INTB1, INPUT_PULLUP);
  attachInterrupt(INTB1, finishh, FALLING);
  interrupts();
}

double tof()
{
  double diff = (double)read3(TDCx_CALIBRATION2);
  diff -= (double)read3(TDCx_CALIBRATION1);
  diff /= (10.0 - 1.0);
  diff = (1000.0 / TDC_CLK) / diff;
  diff *= (double)read3(TDCx_TIME1);
  return diff;
}

void loop() {
  //Serial.println("looping");
  int32_t testdata;
  int32_t testdata2;
  int32_t testdata3;
  int32_t testdata4;
  int32_t testdata5;
  int32_t testdata6;
  int  test = TDCx_INT_STATUS;
  //testdata = read3(test);
  //testdata=tof();
  /*testdata4 = read1(TDCx_CONFIG1);
    testdata5 = read1(TDCx_COARSE_CNTR_OVF_H);
    testdata6 = read1(TDCx_COARSE_CNTR_OVF_L);
    testdata3 = read3(TDCx_TIME1);
    testdata2 = read3(TDCx_CALIBRATION2);
    Serial.println(testdata, HEX);
    Serial.println(testdata2, HEX);
    Serial.println(testdata3, HEX);
    Serial.println(testdata4, HEX);
    Serial.println(testdata5, HEX);
    Serial.println(testdata6, HEX);*/
  //delay(1000);
  //Serial.println(testdata, BIN);
  timee[0] = micros();
  if (j % 500000 == 0) {
    writeRegister(TDCx_CONFIG1, 0x03);
    Serial.print("writeRegister");
    Serial.print(", j = ");
    Serial.println(j);
  }

  //Serial.print("TRIGG = ");
  //Serial.println(digitalRead(TRIGG));
  digitalWrite(start1pin, HIGH);
  //delayMicroseconds(1);

  //testdata = read1(test);
  //testdata=tof();
  //Serial.println(testdata, BIN);
  //digitalWrite(start1pin, LOW);
  //delay(1000);
  //testdata = analogRead(stop1pin);
  testdata = digitalRead(stop1pin);

  //if (testdata > 10) {
  if (testdata == HIGH) {
    timee[1] = micros();
    timee[2] = timee[1] - timee[0];
    Serial.println(testdata);
    Serial.print("time measured : ");
    Serial.println(timee[2]);
    digitalWrite(sipin, HIGH);
    delay(200);
    digitalWrite(sipin, LOW);
    delay(200);
  }
  Serial.println(testdata);


  //Serial.print("TRIGG = ");
  //Serial.println(digitalRead(TRIGG));
  //digitalWrite(start1pin, HIGH);
  //delayMicroseconds(1);

  //testdata = read1(test);
  //testdata=tof();
  //Serial.println(testdata, BIN);
  //digitalWrite(start1pin, LOW);
  j++;
  //digitalWrite(start1pin, LOW);
  //delay(500);
}
