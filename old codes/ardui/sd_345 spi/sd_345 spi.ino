//Add the SPI library so we can communicate with the ADXL345 sensor
#include <SPI.h>
#include <SD.h>

const int chipSelect = 4;
int CS1 = 10;
int CS2 = 9;
int CS3 = 8;
int CS4 = 7;

int Out;

int i=1;

File logfile;

void setup(){ 




  SPI.begin();
  SPI.setDataMode(SPI_MODE3);
  Serial.begin(9600);

  pinMode(CS1, OUTPUT);
  digitalWrite(CS1, HIGH);

  writeRegister_1(DATA_FORMAT_1, 0x0B);
  writeRegister_1(POWER_CTL_1, 0x08);  //Measurement mode 

  writeRegister_1(XOFFSET_1, -3);
  writeRegister_1(YOFFSET_1, 0);
  writeRegister_1(ZOFFSET_1, 15); 

  pinMode(CS2, OUTPUT);
  digitalWrite(CS2, HIGH);

  writeRegister_2(DATA_FORMAT_2,  0x0B);
  writeRegister_2(POWER_CTL_2, 0x08);  //Measurement mode

  pinMode(CS3, OUTPUT);
  digitalWrite(CS3, HIGH);

  writeRegister_3(DATA_FORMAT_3,  0x0B);
  writeRegister_3(POWER_CTL_3, 0x08);  //Measurement mode

  pinMode(CS4, OUTPUT);
  digitalWrite(CS4, HIGH);

  writeRegister_4(DATA_FORMAT_4,  0x0B);
  writeRegister_4(POWER_CTL_4, 0x08);  //Measurement mode 

  Serial.print("Initializing SD card...");
  pinMode(10, OUTPUT);

  if (!SD.begin(chipSelect)) {
    Serial.println("Card failed, or not present");
    return;
  }
  Serial.println("card initialized.");

}

void loop(){

  readRegister_1(DATAX0_1, 6, values_1);
  x1 = ((int)values_1[1]<<8)|(int)values_1[0] & 0xFF;
  y1 = ((int)values_1[3]<<8)|(int)values_1[2] & 0xFF;
  z1 = ((int)values_1[5]<<8)|(int)values_1[4] & 0xFF;

  readRegister_2(DATAX0_2, 6, values_2);
  x2 = ((int)values_2[1]<<8)|(int)values_2[0] & 0xFF;
  y2 = ((int)values_2[3]<<8)|(int)values_2[2] & 0xFF;
  z2 = ((int)values_2[5]<<8)|(int)values_2[4] & 0xFF;

  readRegister_3(DATAX0_3, 6, values_3);
  x3 = ((int)values_3[1]<<8)|(int)values_3[0] & 0xFF;
  y3 = ((int)values_3[3]<<8)|(int)values_3[2] & 0xFF;
  z3 = ((int)values_3[5]<<8)|(int)values_3[4] & 0xFF;
  readRegister_4(DATAX0_4, 6, values_4);
  x4 = ((int)values_4[1]<<8)|(int)values_4[0] & 0xFF;
  y4 = ((int)values_4[3]<<8)|(int)values_4[2] & 0xFF;
  z4 = ((int)values_4[5]<<8)|(int)values_4[4] & 0xFF;


  xg1 = x1 * 3.637;
  yg1 = y1 * 3.588;
  zg1 = z1 * 4.002;

  xg2 = x2 * 3.6;
  yg2 = y2 * 3.623;
  zg2 = z2 * 3.906;

  xg3 = x3 * 3.65;
  yg3 = y3 * 3.6;
  zg3 = z3 * 3.906;

  xg4 = x4 * 3.633;
  yg4 = y4 * 3.633;
  zg4 = z4 * 3.906;



  String dataString = "";

  dataString += String(i);    
  dataString += ","; 
  dataString += ","; 
  dataString += ","; 
  dataString += String(xg1);    
  dataString += ",";  
  dataString += String(yg1);    
  dataString += ","; 
  dataString += String(zg1);
  dataString += ","; 
  dataString += ","; 
  dataString += ","; 
  dataString += String(xg2);    
  dataString += ",";  
  dataString += String(yg2);    
  dataString += ","; 
  dataString += String(zg2);
  dataString += ","; 
  dataString += ","; 
  dataString += ","; 
  dataString += String(xg3);    
  dataString += ",";  
  dataString += String(yg3);    
  dataString += ","; 
  dataString += String(zg3);
  dataString += ","; 
  dataString += ","; 
  dataString += ","; 
  dataString += String(xg4);    
  dataString += ",";  
  dataString += String(yg4);    
  dataString += ","; 
  dataString += String(zg4);
  
  File dataFile = SD.open("datalog.txt", FILE_WRITE);

  if (dataFile) {
    dataFile.println(dataString);
    dataFile.close();
    // print to the serial port too:
    Serial.println(dataString);
  }  
  else {
    Serial.println("error opening datalog.txt");
  } 

  i++;


  delay(200); 
}


void writeRegister_1(char registerAddress_1, char value_1){
  digitalWrite(CS1, LOW);
  SPI.transfer(registerAddress_1);
  SPI.transfer(value_1);
  digitalWrite(CS1, HIGH);
}

void writeRegister_2(char registerAddress_2, char value_2){
  digitalWrite(CS2, LOW);
  SPI.transfer(registerAddress_2);
  SPI.transfer(value_2);
  digitalWrite(CS2, HIGH);
}

void writeRegister_3(char registerAddress_3, char value_3){
  digitalWrite(CS3, LOW);
  SPI.transfer(registerAddress_3);
  SPI.transfer(value_3);
  digitalWrite(CS3, HIGH);
}

void writeRegister_4(char registerAddress_4, char value_4){
  digitalWrite(CS4, LOW);
  SPI.transfer(registerAddress_4);
  SPI.transfer(value_4);
  digitalWrite(CS4, HIGH);
}

// Accel 1

void readRegister_1(char registerAddress_1, int numBytes_1, char * values_1){
  char address_1 = 0x80 | registerAddress_1;
  if(numBytes_1 > 1)address_1 = address_1 | 0x40;

  digitalWrite(CS1, LOW);
  SPI.transfer(address_1);
  for(int a=0; a<numBytes_1; a++){
    values_1[a] = SPI.transfer(0x00);
  }
  digitalWrite(CS1, HIGH);
}

// Accel 2

void readRegister_2(char registerAddress_2, int numBytes_2, char * values_2){
  char address_2 = 0x80 | registerAddress_2;
  if(numBytes_2 > 1)address_2 = address_2 | 0x40;

  digitalWrite(CS2, LOW);
  SPI.transfer(address_2);
  for(int a=0; a<numBytes_2; a++){
    values_2[a] = SPI.transfer(0x00);
  }
  digitalWrite(CS2, HIGH);
}

void readRegister_3(char registerAddress_3, int numBytes_3, char * values_3){
  char address_3 = 0x80 | registerAddress_3;
  if(numBytes_3 > 1)address_3= address_3 | 0x40;

  digitalWrite(CS3, LOW);
  SPI.transfer(address_3);
  for(int c=0; c<numBytes_3; c++){
    values_3[c] = SPI.transfer(0x00);
  }
  digitalWrite(CS3, HIGH);
}



void readRegister_4(char registerAddress_4, int numBytes_4, char * values_4){
  char address_4 = 0x80 | registerAddress_4;
  if(numBytes_4 > 1)address_4 = address_4 | 0x40;

  digitalWrite(CS4, LOW);
  SPI.transfer(address_4);
  for(int d=0; d<numBytes_4; d++){
    values_4[d] = SPI.transfer(0x00);
  }
  digitalWrite(CS4, HIGH);
}



































//Add the SPI library so we can communicate with the ADXL345 sensor
#include <SPI.h>
#include <SD.h>

const int chipSelect = 4;

//Assign the Chip Select signal to pin 10.
int CS1 = 10;
int CS2 = 9;
int CS3 = 8;
int CS4 = 7;

int Out;

int i=1;

int xg1, yg1, zg1;
int xg2, yg2, zg2;
int xg3, yg3, zg3;
int xg4, yg4, zg4;

File logfile;

//This buffer will hold values read from the ADXL345 registers.
char values_1[10];
char values_2[10];
char values_3[10];
char values_4[10];

//These variables will be used to hold the x,y and z axis accelerometer values.
int x1,y1,z1;
int x2,y2,z2;
int x3,y3,z3;
int x4,y4,z4;

//This is a list of some of the registers available on the ADXL345.
//To learn more about these and the rest of the registers on the ADXL345, read the datasheet!
char POWER_CTL_1 = 0x2D;   //Power Control Register
char DATA_FORMAT_1 = 0x31;
char DATAX0_1 = 0x32;   //X-Axis Data 0
char DATAX1_1 = 0x33;   //X-Axis Data 1
char DATAY0_1 = 0x34;   //Y-Axis Data 0
char DATAY1_1 = 0x35;   //Y-Axis Data 1
char DATAZ0_1 = 0x36;   //Z-Axis Data 0
char DATAZ1_1 = 0x37;   //Z-Axis Data 1
char XOFFSET_1 = 0x1E;   //X-Offset
char YOFFSET_1 = 0x1F;   //Y-Offset
char ZOFFSET_1 = 0x20;   //Z-Offset



char POWER_CTL_2 = 0x2D;   //Power Control Register
char DATA_FORMAT_2 = 0x31;
char DATAX0_2 = 0x32;   //X-Axis Data 0
char DATAX1_2 = 0x33;   //X-Axis Data 1
char DATAY0_2 = 0x34;   //Y-Axis Data 0
char DATAY1_2 = 0x35;   //Y-Axis Data 1
char DATAZ0_2 = 0x36;   //Z-Axis Data 0
char DATAZ1_2 = 0x37;   //Z-Axis Data 1
char XOFFSET_2 = 0x1E;   //X-Offset
char YOFFSET_2 = 0x1F;   //Y-Offset
char ZOFFSET_2 = 0x20;   //Z-Offset


char POWER_CTL_3 = 0x2D;   //Power Control Register
char DATA_FORMAT_3 = 0x31;
char DATAX0_3 = 0x32;   //X-Axis Data 0
char DATAX1_3 = 0x33;   //X-Axis Data 1
char DATAY0_3 = 0x34;   //Y-Axis Data 0
char DATAY1_3 = 0x35;   //Y-Axis Data 1
char DATAZ0_3 = 0x36;   //Z-Axis Data 0
char DATAZ1_3 = 0x37;   //Z-Axis Data 1
char XOFFSET_3 = 0x1E;   //X-Offset
char YOFFSET_3 = 0x1F;   //Y-Offset
char ZOFFSET_3= 0x20;   //Z-Offset


char POWER_CTL_4 = 0x2D;   //Power Control Register
char DATA_FORMAT_4 = 0x31;
char DATAX0_4 = 0x32;   //X-Axis Data 0
char DATAX1_4 = 0x33;   //X-Axis Data 1
char DATAY0_4 = 0x34;   //Y-Axis Data 0
char DATAY1_4 = 0x35;   //Y-Axis Data 1
char DATAZ0_4 = 0x36;   //Z-Axis Data 0
char DATAZ1_4 = 0x37;   //Z-Axis Data 1
char XOFFSET_4 = 0x1E;   //X-Offset
char YOFFSET_4 = 0x1F;   //Y-Offset
char ZOFFSET_4 = 0x20;   //Z-Offset




void setup(){ 





  //Initiate an SPI communication instance.
  SPI.begin();
  //Configure the SPI connection for the ADXL345.
  SPI.setDataMode(SPI_MODE3);
  //Create a serial connection to display the data on the terminal.
  Serial.begin(9600);


  //  Accel 1
  //Set up the Chip Select pin to be an output from the Arduino.
  pinMode(CS1, OUTPUT);
  //Before communication starts, the Chip Select pin needs to be set high.
  digitalWrite(CS1, HIGH);

  //Put the ADXL345 into +/- 4G range by writing the value 0x01 to the DATA_FORMAT register.
  writeRegister_1(DATA_FORMAT_1, 0x0B);
  //Put the ADXL345 into Measurement Mode by writing 0x08 to the POWER_CTL register.
  writeRegister_1(POWER_CTL_1, 0x08);  //Measurement mode 

  writeRegister_1(XOFFSET_1, -3);
  writeRegister_1(YOFFSET_1, 0);
  writeRegister_1(ZOFFSET_1, 15); 

  //  Accel 2
  //Set up the Chip Select pin to be an output from the Arduino.
  pinMode(CS2, OUTPUT);
  //Before communication starts, the Chip Select pin needs to be set high.
  digitalWrite(CS2, HIGH);

  //Put the ADXL345 into +/- 4G range by writing the value 0x01 to the DATA_FORMAT register.
  writeRegister_2(DATA_FORMAT_2,  0x0B);
  //Put the ADXL345 into Measurement Mode by writing 0x08 to the POWER_CTL register.
  writeRegister_2(POWER_CTL_2, 0x08);  //Measurement mode

  writeRegister_2(XOFFSET_2, -5);
  writeRegister_2(YOFFSET_2, -3);
  writeRegister_2(ZOFFSET_2, 7);  

  //  Accel 3
  //Set up the Chip Select pin to be an output from the Arduino.
  pinMode(CS3, OUTPUT);
  //Before communication starts, the Chip Select pin needs to be set high.
  digitalWrite(CS3, HIGH);

  //Put the ADXL345 into +/- 4G range by writing the value 0x01 to the DATA_FORMAT register.
  writeRegister_3(DATA_FORMAT_3,  0x0B);
  //Put the ADXL345 into Measurement Mode by writing 0x08 to the POWER_CTL register.
  writeRegister_3(POWER_CTL_3, 0x08);  //Measurement mode

  writeRegister_3(XOFFSET_3, -4);
  writeRegister_3(YOFFSET_3, -3);
  writeRegister_3(ZOFFSET_3, 7); 

  //  Accel 4
  //Set up the Chip Select pin to be an output from the Arduino.
  pinMode(CS4, OUTPUT);
  //Before communication starts, the Chip Select pin needs to be set high.
  digitalWrite(CS4, HIGH);

  //Put the ADXL345 into +/- 4G range by writing the value 0x01 to the DATA_FORMAT register.
  writeRegister_4(DATA_FORMAT_4,  0x0B);
  //Put the ADXL345 into Measurement Mode by writing 0x08 to the POWER_CTL register.
  writeRegister_4(POWER_CTL_4, 0x08);  //Measurement mode 

  writeRegister_4(XOFFSET_4, -1);
  writeRegister_4(YOFFSET_4, -3);
  writeRegister_4(ZOFFSET_4, 6);


  Serial.print("Initializing SD card...");
  // make sure that the default chip select pin is set to
  // output, even if you don't use it:
  pinMode(10, OUTPUT);

  // see if the card is present and can be initialized:
  if (!SD.begin(chipSelect)) {
    Serial.println("Card failed, or not present");
    // don't do anything more:
    return;
  }
  Serial.println("card initialized.");

}

void loop(){

  // Accel 1

  //Reading 6 bytes of data starting at register DATAX0 will retrieve the x,y and z acceleration values from the ADXL345.
  //The results of the read operation will get stored to the values[] buffer.
  readRegister_1(DATAX0_1, 6, values_1);

  //The ADXL345 gives 10-bit acceleration values, but they are stored as bytes (8-bits). To get the full value, two bytes must be combined for each axis.
  //The X value is stored in values[0] and values[1].
  x1 = ((int)values_1[1]<<8)|(int)values_1[0] & 0xFF;
  //The Y value is stored in values[2] and values[3].
  y1 = ((int)values_1[3]<<8)|(int)values_1[2] & 0xFF;
  //The Z value is stored in values[4] and values[5].
  z1 = ((int)values_1[5]<<8)|(int)values_1[4] & 0xFF;

  // Accel 2

  //Reading 6 bytes of data starting at register DATAX0 will retrieve the x,y and z acceleration values from the ADXL345.
  //The results of the read operation will get stored to the values[] buffer.
  readRegister_2(DATAX0_2, 6, values_2);

  //The ADXL345 gives 10-bit acceleration values, but they are stored as bytes (8-bits). To get the full value, two bytes must be combined for each axis.
  //The X value is stored in values[0] and values[1].
  x2 = ((int)values_2[1]<<8)|(int)values_2[0] & 0xFF;
  //The Y value is stored in values[2] and values[3].
  y2 = ((int)values_2[3]<<8)|(int)values_2[2] & 0xFF;
  //The Z value is stored in values[4] and values[5].
  z2 = ((int)values_2[5]<<8)|(int)values_2[4] & 0xFF;

  // Accel 3

  //Reading 6 bytes of data starting at register DATAX0 will retrieve the x,y and z acceleration values from the ADXL345.
  //The results of the read operation will get stored to the values[] buffer.
  readRegister_3(DATAX0_3, 6, values_3);

  //The ADXL345 gives 10-bit acceleration values, but they are stored as bytes (8-bits). To get the full value, two bytes must be combined for each axis.
  //The X value is stored in values[0] and values[1].
  x3 = ((int)values_3[1]<<8)|(int)values_3[0] & 0xFF;
  //The Y value is stored in values[2] and values[3].
  y3 = ((int)values_3[3]<<8)|(int)values_3[2] & 0xFF;
  //The Z value is stored in values[4] and values[5].
  z3 = ((int)values_3[5]<<8)|(int)values_3[4] & 0xFF;

  // Accel 4

  //Reading 6 bytes of data starting at register DATAX0 will retrieve the x,y and z acceleration values from the ADXL345.
  //The results of the read operation will get stored to the values[] buffer.
  readRegister_4(DATAX0_4, 6, values_4);

  //The ADXL345 gives 10-bit acceleration values, but they are stored as bytes (8-bits). To get the full value, two bytes must be combined for each axis.
  //The X value is stored in values[0] and values[1].
  x4 = ((int)values_4[1]<<8)|(int)values_4[0] & 0xFF;
  //The Y value is stored in values[2] and values[3].
  y4 = ((int)values_4[3]<<8)|(int)values_4[2] & 0xFF;
  //The Z value is stored in values[4] and values[5].
  z4 = ((int)values_4[5]<<8)|(int)values_4[4] & 0xFF;


  xg1 = x1 * 3.637;
  yg1 = y1 * 3.588;
  zg1 = z1 * 4.002;

  xg2 = x2 * 3.6;
  yg2 = y2 * 3.623;
  zg2 = z2 * 3.906;

  xg3 = x3 * 3.65;
  yg3 = y3 * 3.6;
  zg3 = z3 * 3.906;

  xg4 = x4 * 3.633;
  yg4 = y4 * 3.633;
  zg4 = z4 * 3.906;



  String dataString = "";

  //  // read three sensors and append to the string:
  //  for (int analogPin = 0; analogPin < 3; analogPin++) {
  //    int sensor = analogRead(analogPin);

  dataString += String(i);    
  dataString += ","; 
  dataString += ","; 
  dataString += ","; 
  dataString += String(xg1);    
  dataString += ",";  
  dataString += String(yg1);    
  dataString += ","; 
  dataString += String(zg1);
  dataString += ","; 
  dataString += ","; 
  dataString += ","; 
  dataString += String(xg2);    
  dataString += ",";  
  dataString += String(yg2);    
  dataString += ","; 
  dataString += String(zg2);
  dataString += ","; 
  dataString += ","; 
  dataString += ","; 
  dataString += String(xg3);    
  dataString += ",";  
  dataString += String(yg3);    
  dataString += ","; 
  dataString += String(zg3);
  dataString += ","; 
  dataString += ","; 
  dataString += ","; 
  dataString += String(xg4);    
  dataString += ",";  
  dataString += String(yg4);    
  dataString += ","; 
  dataString += String(zg4);

  // open the file. note that only one file can be open at a time,
  // so you have to close this one before opening another.
  File dataFile = SD.open("datalog.txt", FILE_WRITE);

  // if the file is available, write to it:
  if (dataFile) {
    dataFile.println(dataString);
    dataFile.close();
    // print to the serial port too:
    Serial.println(dataString);
  }  
  // if the file isn't open, pop up an error:
  else {
    Serial.println("error opening datalog.txt");
  } 

  i++;


  delay(200); 
}


//This function will write a value to a register on the ADXL345.
//Parameters:
//  char registerAddress - The register to write a value to
//  char value - The value to be written to the specified register.
void writeRegister_1(char registerAddress_1, char value_1){
  //Set Chip Select pin low to signal the beginning of an SPI packet.
  digitalWrite(CS1, LOW);
  //Transfer the register address over SPI.
  SPI.transfer(registerAddress_1);
  //Transfer the desired register value over SPI.
  SPI.transfer(value_1);
  //Set the Chip Select pin high to signal the end of an SPI packet.
  digitalWrite(CS1, HIGH);
}

void writeRegister_2(char registerAddress_2, char value_2){
  //Set Chip Select pin low to signal the beginning of an SPI packet.
  digitalWrite(CS2, LOW);
  //Transfer the register address over SPI.
  SPI.transfer(registerAddress_2);
  //Transfer the desired register value over SPI.
  SPI.transfer(value_2);
  //Set the Chip Select pin high to signal the end of an SPI packet.
  digitalWrite(CS2, HIGH);
}

void writeRegister_3(char registerAddress_3, char value_3){
  //Set Chip Select pin low to signal the beginning of an SPI packet.
  digitalWrite(CS3, LOW);
  //Transfer the register address over SPI.
  SPI.transfer(registerAddress_3);
  //Transfer the desired register value over SPI.
  SPI.transfer(value_3);
  //Set the Chip Select pin high to signal the end of an SPI packet.
  digitalWrite(CS3, HIGH);
}

void writeRegister_4(char registerAddress_4, char value_4){
  //Set Chip Select pin low to signal the beginning of an SPI packet.
  digitalWrite(CS4, LOW);
  //Transfer the register address over SPI.
  SPI.transfer(registerAddress_4);
  //Transfer the desired register value over SPI.
  SPI.transfer(value_4);
  //Set the Chip Select pin high to signal the end of an SPI packet.
  digitalWrite(CS4, HIGH);
}

// Accel 1

//This function will read a certain number of registers starting from a specified address and store their values in a buffer.
//Parameters:
//  char registerAddress - The register addresse to start the read sequence from.
//  int numBytes - The number of registers that should be read.
//  char * values - A pointer to a buffer where the results of the operation should be stored.
void readRegister_1(char registerAddress_1, int numBytes_1, char * values_1){
  //Since we're performing a read operation, the most significant bit of the register address should be set.
  char address_1 = 0x80 | registerAddress_1;
  //If we're doing a multi-byte read, bit 6 needs to be set as well.
  if(numBytes_1 > 1)address_1 = address_1 | 0x40;

  //Set the Chip select pin low to start an SPI packet.
  digitalWrite(CS1, LOW);
  //Transfer the starting register address that needs to be read.
  SPI.transfer(address_1);
  //Continue to read registers until we've read the number specified, storing the results to the input buffer.
  for(int a=0; a<numBytes_1; a++){
    values_1[a] = SPI.transfer(0x00);
  }
  //Set the Chips Select pin high to end the SPI packet.
  digitalWrite(CS1, HIGH);
}

// Accel 2

void readRegister_2(char registerAddress_2, int numBytes_2, char * values_2){
  //Since we're performing a read operation, the most significant bit of the register address should be set.
  char address_2 = 0x80 | registerAddress_2;
  //If we're doing a multi-byte read, bit 6 needs to be set as well.
  if(numBytes_2 > 1)address_2 = address_2 | 0x40;

  //Set the Chip select pin low to start an SPI packet.
  digitalWrite(CS2, LOW);
  //Transfer the starting register address that needs to be read.
  SPI.transfer(address_2);
  //Continue to read registers until we've read the number specified, storing the results to the input buffer.
  for(int a=0; a<numBytes_2; a++){
    values_2[a] = SPI.transfer(0x00);
  }
  //Set the Chips Select pin high to end the SPI packet.
  digitalWrite(CS2, HIGH);
}

void readRegister_3(char registerAddress_3, int numBytes_3, char * values_3){
  //Since we're performing a read operation, the most significant bit of the register address should be set.
  char address_3 = 0x80 | registerAddress_3;
  //If we're doing a multi-byte read, bit 6 needs to be set as well.
  if(numBytes_3 > 1)address_3= address_3 | 0x40;

  //Set the Chip select pin low to start an SPI packet.
  digitalWrite(CS3, LOW);
  //Transfer the starting register address that needs to be read.
  SPI.transfer(address_3);
  //Continue to read registers until we've read the number specified, storing the results to the input buffer.
  for(int c=0; c<numBytes_3; c++){
    values_3[c] = SPI.transfer(0x00);
  }
  //Set the Chips Select pin high to end the SPI packet.
  digitalWrite(CS3, HIGH);
}



void readRegister_4(char registerAddress_4, int numBytes_4, char * values_4){
  //Since we're performing a read operation, the most significant bit of the register address should be set.
  char address_4 = 0x80 | registerAddress_4;
  //If we're doing a multi-byte read, bit 6 needs to be set as well.
  if(numBytes_4 > 1)address_4 = address_4 | 0x40;

  //Set the Chip select pin low to start an SPI packet.
  digitalWrite(CS4, LOW);
  //Transfer the starting register address that needs to be read.
  SPI.transfer(address_4);
  //Continue to read registers until we've read the number specified, storing the results to the input buffer.
  for(int d=0; d<numBytes_4; d++){
    values_4[d] = SPI.transfer(0x00);
  }
  //Set the Chips Select pin high to end the SPI packet.
  digitalWrite(CS4, HIGH);
}
