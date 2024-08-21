////////////////////////////////////////////////////////////////////////////////////////////////////////
//  November 2017
//  Author: Juan Jose Chong <juan.chong@analog.com>
////////////////////////////////////////////////////////////////////////////////////////////////////////
//  ADIS16470_Teensy_BurstRead_Datalog_Example.ino
////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//  This Arduino project interfaces with an ADIS16470 using SPI and the
//  accompanying C++ libraries, reads IMU data in LSBs, and transmits
//  measurements to a serial debug terminal (PuTTY) via the onboard
//  USB serial port using Serial.write().
//
//  This project has been tested on a PJRC 32-Bit Teensy 3.2 Development Board,
//  but should be compatible with any other embedded platform with some modification.
//
//  Permission is hereby granted, free of charge, to any person obtaining
//  a copy of this software and associated documentation files (the
//  "Software"), to deal in the Software without restriction, including
//  without limitation the rights to use, copy, modify, merge, publish,
//  distribute, sublicense, and/or sell copies of the Software, and to
//  permit persons to whom the Software is furnished to do so, subject to
//  the following conditions:
//
//  The above copyright notice and this permission notice shall be
//  included in all copies or substantial portions of the Software.
//
//  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
//  EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
//  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
//  NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
//  LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
//  OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
//  WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
//
//  Pinout for a Teensy 3.2 Development Board
//  RST = D6
//  SCK = D13/SCK
//  CS = D10/CS
//  DOUT(MISO) = D12/MISO
//  DIN(MOSI) = D11/MOSI
//  DR = D2
//
////////////////////////////////////////////////////////////////////////////////////////////////////////
#include <ADIS16470.h>
#include <SPI.h>

// Initialize Variables
// Temporary Data Array
uint8_t *burstData;
int32_t my[10];
double az;

// Call ADIS16470 Class
ADIS16470 IMU(10, 2, 6); // Chip Select, Data Ready, Reset Pin Assignments

void setup()
{
  Serial.begin(115200); // Initialize serial output via USB
  IMU.configSPI(); // Configure SPI communication
  delay(1000); // Give the part time to start up
  IMU.regWrite(MSC_CTRL, 0xC1);  // Enable Data Ready, set polarity
  IMU.regWrite(DEC_RATE, 0x00); // Set digital filter
  IMU.regWrite(FILT_CTRL, 0x00); // Set digital filter //DEC_RATE 0x00 FILT_CTRL 0x00 acc=20bit
}

// Function used to read register values when an ISR is triggered from the IMU's Data Ready pin and print to serial port
void printData()
{
  burstData = IMU.byteBurst(); // Read burst data and point to data array. Disable checksum
  /*Serial.write(0xA5); // Frame delimiter
    Serial.write(0xA5); // Frame delimiter
    Serial.write(0xA5); // Frame delimiter
    Serial.write(burstData,20); // Push data pointer to serial port
    delay(100); // Give the part time to start up*/
}

// Main loop
void loop()
{
  attachInterrupt(2, printData, RISING); // Attach interrupt to pin 2. Trigger on the rising edge

  /*Serial.print(*(burstData));
    Serial.print("\t");
    Serial.print(*(burstData + 1));
    Serial.print("\t");
    Serial.print(*(burstData + 2));
    Serial.print("\t");
    Serial.print(*(burstData + 3));
    Serial.print("\t");
    Serial.print(*(burstData + 4));
    Serial.print("\t");
    Serial.print(*(burstData + 5));
    Serial.print("\t");
    Serial.print(*(burstData + 6));
    Serial.print("\t");    */

  /* Serial.print(burstData[0],BIN);
    Serial.print("\t");
    Serial.print(burstData[1],BIN);
    Serial.print("\t");
    Serial.print(burstData[2],BIN);
    Serial.print("\t");
    Serial.print(burstData[3],BIN);
    Serial.print("\t");
    Serial.print(burstData[4],BIN);
    Serial.print("\t");
    Serial.print(burstData[5],BIN);
    Serial.print("\t");
    Serial.print(burstData[6],BIN);
    Serial.print("\t");*/

  my[0] = burstData[12];
  my[1] = burstData[13];
  if (my[0] > 0x7F) {
    my[0] = my[0] - 0xFF;
  }

  my[0] = my[0] << 8;
  my[1] = my[1];

  my[9] = my[0] | my[1];// | 0x0F;
  az = my[9];
  az = az *0.00025;

  Serial.print(az, 6);
  Serial.print("\t");

  Serial.print(my[9]);
  Serial.print("\t");
  Serial.print(my[9], BIN);
  Serial.print("\t");

  Serial.print(my[0]);
  Serial.print("\t");
  Serial.print(my[0], BIN);
  Serial.print("\t");

  Serial.print(my[1]);
  Serial.print("\t");
  Serial.print(my[1], BIN);
  Serial.print("\t");

  /*Serial.print(burstData[12]);
    Serial.print("\t");
    Serial.print(burstData[13]);
    Serial.print("\t");

    Serial.print(burstData[12],BIN);
    Serial.print("\t");
    Serial.print(burstData[13],BIN);
    Serial.print("\t");*/


  Serial.print("\n");
  delay(10);
}
