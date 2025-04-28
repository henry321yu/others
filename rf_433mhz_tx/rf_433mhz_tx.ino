/*
  433 MHz RF Module Transmitter Demonstration 1
  RF-Xmit-Demo-1.ino
  Demonstrates 433 MHz RF Transmitter Module
  Use with Receiver Demonstration 1

  DroneBot Workshop 2018
  https://dronebotworkshop.com
*/

// Include RadioHead Amplitude Shift Keying Library
#include <RH_ASK.h>
// Include dependant SPI Library 
#include <SPI.h> 

// Create ASK Object with custom pins: TX=D3, RX=D4, PTT=D5
RH_ASK rf_driver(2000, 4, 3, 5);

void setup()
{
    // Initialize ASK Object
    rf_driver.init();
}

void loop()
{
    const char *msg = "Welcome to the Workshop!";
    rf_driver.send((uint8_t *)msg, strlen(msg));
    rf_driver.waitPacketSent();
    delay(1000);
}
