
/* Include DigitLedDisplay Library */
#include "DigitLedDisplay.h"
#include <SPI.h>

/* Arduino Pin to Display Pin
   7 to DIN,
   6 to CS,
   5 to CLK */
DigitLedDisplay ld = DigitLedDisplay(9, 8, 7);
byte flag = 1;

void setup() {
  Serial.begin(115200);
  /* Set the brightness min:1, max:15 */
  ld.setBright(15);

  /* Set the digit count */
  ld.setDigitLimit(8);

}

void loop() {
  double t = millis();
  t = 110 - t / 1000;
  if (flag == 1) {
    if (t < 100) {
      ld.clear();
      flag = 0;
    }
  }

  ld.printDigit(t);
  Serial.println(t);
  delay(500);

  ld.printDigit(t, 4);
  Serial.println(t);
  delay(500);
}
