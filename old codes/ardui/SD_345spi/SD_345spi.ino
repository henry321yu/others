///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//  Sonny Schade   04/15/2014
//  Accelerometer Project using SparkFun ADXL345 and SD Data Logger Breakout boards
//  Both Sheilds communicating over SPI
//  SD Card uses Pin 8 by default for SS/CS Pin (Spark Fun Board)
//  ADXL345 using Pin 10 for CS pin but you can use another digital pin if you choose
//  
//  This code was created by mutilating and sewing together code from sample code and other people's projects 
//  so feel free to do the same to this one, I hope this helps someone starting out.
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Include the relevant libraries provided with the Arduino
#include <SPI.h>
#include <SD.h>

// place holder for a large number
unsigned long time; // called it "time"

//Factors to change the data output by the accelerometer from raw data to G forces
double g=0.0039; //+/- 2g Make sure this matches the writeRegister Data Format on line 91!
//double g=0.0078; //+/- 4g
//double g=0.016;  //+/- 8g
//double g=0.031; //+/- 16g

#define SYNC_INTERVAL 10000 // mills between calls to flush() - to write data to the card
uint32_t syncTime = 0; // time of last sync()

// Assign the ADXL345 CS pin to Pin 10 ** make this whatever digital pin you chose to use**
int CS=10;

//This is a list of some of the registers available on the ADXL345.
//To learn more about these and the rest of the registers on the ADXL345, read the datasheet!
char POWER_CTL = 0x2D;    //Power Control Register
char DATA_FORMAT = 0x31;
char DATAX0 = 0x32;    //X-Axis Data 0
char DATAX1 = 0x33;    //X-Axis Data 1
char DATAY0 = 0x34;    //Y-Axis Data 0
char DATAY1 = 0x35;    //Y-Axis Data 1
char DATAZ0 = 0x36;    //Z-Axis Data 0
char DATAZ1 = 0x37;    //Z-Axis Data 1
char BW_RATE = 0x2C;    //Sampling Rate

//This buffer will hold values read from the ADXL345 registers.
char values[10];
//Initalize variables
int x,y,z;
int i;
double xg,yg,zg;
double totals[3];
File myFile;

void setup(){
  //Initiate an SPI communication instance.
  SPI.begin();
  Serial.begin(38400);   
  
  //Set up the Chip Select pin for the SD card to be an output from the Arduino.
  pinMode(8, OUTPUT);
  //Set chip select for ADXL345 to be output from Arduino.
  pinMode(CS, OUTPUT);

  //Set the CS Pin for the accelerometer high so that the ADXL345 and SD card won't be trying to talk at same time.
  digitalWrite(CS, HIGH);

  //Initialize the SD card
  if (!SD.begin(8)) { // the (8) is the CS pin you chose to use, on my sheild that pin is hardwired to "8"
    return;
  }
  // create a new file 
  // starts with Crash00, and next one would be Crash01 
  char filename[] = "Crash.txt";
  for (uint8_t i = 0; i < 100; i++) {
    filename[6] = i/10 + '0';
    filename[7] = i%10 + '0';
    if (! SD.exists(filename)) {
      // only open a new file if it doesn't exist
      myFile = SD.open(filename, FILE_WRITE);
      break;  // leave the loop!
    }
  }
   
  //Print header to file
  myFile.println("Time in MilliSeconds,X_Axis,Y_Axis,Z_Axis, X_G's,Y_G's,Z_G's");

  //Configure the SPI connection for the ADXL345.
  SPI.setDataMode(SPI_MODE3);
  //Before communication starts, the Chip Select pin needs to be set high.
  digitalWrite(CS, HIGH);

  //Put the ADXL345 into +/- 4G range by writing the value 0x01 to the DATA_FORMAT register.
  writeRegister(DATA_FORMAT, 0x00);//0x00 +/- 2G, 0x01 +/- 4G. 0x10 +/- 8G,0x11 +/-16G
  //Put the ADXL345 into Measurement Mode by writing 0x08 to the POWER_CTL register.
  writeRegister(POWER_CTL, 0x08);  //Measurement mode 
  //Try and change sampling rate
  writeRegister(BW_RATE, 0x0D);
}

void loop(){

  //Count Milliseconds from time Arduino powered on
  time = millis();
  
  //Reading 6 bytes of data starting at register DATAX0 will retrieve the x,y and z acceleration values from the ADXL345.
  //The results of the read operation will get stored to the values[] buffer.
  readRegister(DATAX0, 6, values);

  //The ADXL345 gives 10-bit acceleration values, but they are stored as bytes (8-bits). To get the full value, two bytes must be combined for each axis.
  //The X value is stored in values[0] and values[1].
  x = ((int)values[1]<<8)|(int)values[0];
  //The Y value is stored in values[2] and values[3].
  y = ((int)values[3]<<8)|(int)values[2];
  //The Z value is stored in values[4] and values[5].
  z = ((int)values[5]<<8)|(int)values[4];

  //Convert raw data to g values
  xg=x*g;
  yg=y*g;
  zg=z*g;

  //Store g values in an array
  totals[0] = xg;
  totals[1] = yg;
  totals[2] = zg;

   // log time
    myFile.print(time);
    myFile.print(',');
   
  //Log accelerations
    myFile.print(x);
    myFile.print(',');
    myFile.print(y);
    myFile.print(',');
    myFile.print(z);
    myFile.print(',');
  for (i = 0; i < 3; i = i + 1) {
    myFile.print(totals[i]);
    myFile.print(',');
    // Open the serial monitor to see if it's working, select a baud rate of 38400, only displays milliseconds and G's
    Serial.print(time);
    Serial.print(',');
    Serial.print(totals[i]);
    Serial.print(',');
  }
  myFile.println();
  Serial.println();
 
  // Now we write data to disk! Don't sync too often - requires 2048 bytes of I/O to SD card
  // which uses a bunch of power and takes time
  if ((millis() - syncTime) < SYNC_INTERVAL) return;
  syncTime = millis();

  // flush the system
  myFile.flush();
 // delay to help clear some of the noise
 // delayMicroseconds(2);

}   //END OF LOOP!!!!!

//This function will write a value to a register on the ADXL345.
//Parameters:
//  char registerAddress - The register to write a value to
//  char value - The value to be written to the specified register.
void writeRegister(char registerAddress, char value){
  //Set Chip Select pin low to signal the beginning of an SPI packet.
  digitalWrite(CS, LOW);
  //Transfer the register address over SPI.
  SPI.transfer(registerAddress);
  //Transfer the desired register value over SPI.
  SPI.transfer(value);
  //Set the Chip Select pin high to signal the end of an SPI packet.
  digitalWrite(CS, HIGH);
}

//This function will read a certain number of registers starting from a specified address and store their values in a buffer.
//Parameters:
//  char registerAddress - The register addresse to start the read sequence from.
//  int numBytes - The number of registers that should be read.
//  char * values - A pointer to a buffer where the results of the operation should be stored.
void readRegister(char registerAddress, int numBytes, char * values){
  //Since we're performing a read operation, the most significant bit of the register address should be set.
  char address = 0x80 | registerAddress;
  //If we're doing a multi-byte read, bit 6 needs to be set as well.
  if(numBytes > 1)address = address | 0x40;

  //Set the Chip select pin low to start an SPI packet.
  digitalWrite(CS, LOW);
  //Transfer the starting register address that needs to be read.
  SPI.transfer(address);
  //Continue to read registers until we've read the number specified, storing the results to the input buffer.
  for(int i=0; i<numBytes; i++){
    values[i] = SPI.transfer(0x00);
  }
  //Set the Chips Select pin high to end the SPI packet.
  digitalWrite(CS, HIGH);
}
