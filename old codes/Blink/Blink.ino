//#include <Arduino.h>

/*
  Blink
  Turns on an LED on for one second, then off for one second, repeatedly.

  This example code is in the public domain.
*/

// Pin 13 has an LED connected on most Arduino boards.
// Pin 11 has the LED on Teensy 2.0
// Pin 6  has the LED on Teensy++ 2.0
// Pin 13 has the LED on Teensy 3.0
// give it a name:
//int led = PB12; //stm32 buildin led
int led = PA1;
//int led = 13;
//int led = 19;
unsigned long i = 0;
int t = 100;
int readpin = PA0;
//int readpin = 18;

int m = 1; //mode

int brightness = 0;    // how bright the LED is
int fadeAmount = 5;    // how many points to fade the LED by

// the setup routine runs once when you press reset:
void setup() {
  // initialize the digital pin as an output.
  Serial.begin(115200);
  pinMode(led, OUTPUT);
  pinMode(readpin, INPUT);                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                    
}

// the loop routine runs over and over again forever:
void loop() {
  int32_t readd = analogRead(readpin);

  Serial.println(readd);


  if (m == 1) {
    if (i >= t*20) {
      digitalWrite(led, HIGH);   // turn the LED on (HIGH is the voltage level)
    }
    if (i == 1) {
      digitalWrite(led, LOW);   // turn the LED on (HIGH is the voltage level)
    }
    if (i >= t * 40) {
      i = 0;
    }
    i++;
    delay(1);
  }




  if (m == 2) {
    analogWrite(led, brightness);

    // change the brightness for next time through the loop:
    brightness = brightness + fadeAmount;

    // reverse the direction of the fading at the ends of the fade:
    if (brightness <= 0 || brightness >= 255) {
      fadeAmount = -fadeAmount;
    }
    // wait for 30 milliseconds to see the dimming effect
    delay(5);
  }

}
