#include <SPI.h>

const int FIFO = 0x11;

const int POWER = 0x2D;//1 for standby, 0 for go
const int RANGE = 0x2C;// for range
const int SYNC = 0x2B;
const int STATUS = 0x04; 
const int FILTER = 0x28;
const int chipSelectPin = 10;
const int SELF_TEST = 0x2E;
const int Reset = 0x2F;
int a,b,c,d;

 void setup()  
 {  
  Serial.begin(9600);  
    SPI.begin();
    SPISettings settings(20000000, MSBFIRST, SPI_MODE0);
    SPI.setClockDivider(SPI_CLOCK_DIV16);
    digitalWrite(chipSelectPin, LOW);
    SPI.beginTransaction(settings);
    SPI.transfer(0x01);
    SPI.endTransaction();
    digitalWrite(chipSelectPin, HIGH);
    
    pinMode(chipSelectPin, OUTPUT);
    //writeRegister( Reset, 0x52); // reset
    //writeRegister( POWER, 0x01); // standby
    writeRegister( RANGE, 0x01);// 2g
    //writeRegister( SYNC, 0x00); //internal clock
    //writeRegister( FILTER, 0x00); // 125 Hz
    writeRegister( POWER, 0x00); //measurement mode
    
    writeRegister( SELF_TEST, 0x00); //SELF_TEST mode
    
    delay(100);
 }  
 void loop() { 
            readFIFO();
            a=readRegister(0x1E);
            b=readRegister(0x0F);
            c=readRegister(0x1F);
            Serial.print (a);
            Serial.print ("\t");
            Serial.print (b);
            Serial.print ("\t");
            Serial.println (c);
 }
void readFIFO (){
    //fifo stores 3 bytes of data for each axis, the 20 most significant bits are the values in two's complement
    byte dataByte [9];//
    double dataInt [3];

    digitalWrite(chipSelectPin, LOW);
    SPI.transfer( (FIFO<<1) | 1);
    for (int i = 0 ; i < 9; i++){
        dataByte[i] = SPI.transfer(0x00);
    }
    digitalWrite(chipSelectPin, HIGH);

     // the 16 most significant bits are kept in an integer 
     for (int z = 0 ; z < 3; z++){
        dataInt[z] = ((dataByte[z*3]<<8) | (dataByte[z*3 +1]));         
     }
     /*Serial.print (dataInt[0]);
     Serial.print ("\t");
     Serial.print (dataInt[1]);
     Serial.print ("\t");
     Serial.print (dataInt[2]);
     Serial.print ("\t");
     Serial.print (dataInt[0]/16000,5);
     Serial.print ("\t");
     Serial.print (dataInt[1]/16000,5);
     Serial.print ("\t");
     Serial.println (dataInt[2]/16000,5);*/
     delay(10);
}

 byte readRegister (byte thisRegister){
    byte inByte = 0 ;
    digitalWrite(chipSelectPin, LOW);
    SPI.transfer((thisRegister << 1) | 1);
    inByte = SPI.transfer(0x00);
    digitalWrite(chipSelectPin, HIGH);
    return inByte;
} 
void writeRegister (byte thisRegister, byte value){
    digitalWrite(chipSelectPin, LOW);
    SPI.transfer(thisRegister << 1);
    SPI.transfer(value);
    digitalWrite(chipSelectPin, HIGH);
}
