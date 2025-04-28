/* 1byte 433Mhz RECEIVER example.
/* Tutorial link: http://electronoobs.com/eng_arduino_tut99.php
 * Code: http://electronoobs.com/eng_arduino_tut99_code2.php
 * Scheamtic: http://electronoobs.com/eng_arduino_tut99_sch1.php
 * Youtube Channel: http://www.youtube/c/electronoobs   
// Arduino          433Mhz RX
// GND              GND
// 5V               VCC
// D3               Data
*/

#include <VirtualWire.h>              //Download it here: http://electronoobs.com/eng_arduino_virtualWire.php
byte message[VW_MAX_MESSAGE_LEN]; // a buffer to store the incoming messages
byte messageLength = VW_MAX_MESSAGE_LEN; // the size of the message
int received_number = 0;
int LED = 5;

void setup()
{
  Serial.begin(9600);
  pinMode(LED, OUTPUT);
  Serial.println("Ready...");
  vw_set_rx_pin(3); // pin
  vw_setup(2000); // bps
  vw_rx_start();
}

void loop()
{
  if (vw_get_message(message, &messageLength)) // non-blocking
  {
    Serial.print("Potentiometer: ");
    for (int i = 0; i < messageLength; i++)
    {
      //Serial.print(message[i]);
      received_number = message[i];
    }
    Serial.println(received_number);
    analogWrite(LED, received_number);
  }
}
