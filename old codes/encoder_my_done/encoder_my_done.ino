#include <digitalWriteFast.h>

volatile long temp,  counter = 0; //This variable will increase or decrease depending on the rotation of encoder
int ledd;
double position;
double k = 0.75;
//int H1 = 20, H2 = 21, H3, h4;
int H1 = 14, H2 = 16, H3 = 18, h4;
void setup() {
  Serial.begin (115200);

  pinModeFast(H1, INPUT_PULLUP);
  pinModeFast(H2, INPUT_PULLUP);
  pinModeFast(H3, INPUT_PULLUP);
  attachInterrupt(H1, phaseA, CHANGE);
  attachInterrupt(H2, phaseB, CHANGE);
  attachInterrupt(H3, phaseC, CHANGE);
}

void loop() {
  if ( counter != temp ) {
    Serial.print (counter);
    Serial.print ('\t');
    Serial.println (position);
    temp = counter;

    ledd = map(position, 0, 360, 0, 255);
    analogWrite(14, ledd);
    }
}

void phaseA() {
  if (digitalReadFast(H1) == HIGH) {
    if (digitalReadFast(H2) == LOW) {
      counter++;
      position = position + k;
    } else {
      counter--;
      position = position - k;
    }
  }
  if (digitalReadFast(H1) == LOW) {
    if (digitalReadFast(H2) == HIGH) {
      counter++;
      position = position + k;
    } else {
      counter--;
      position = position - k;
    }
  }
  if (position == -k) {
    position = 360-k;
  }
  if (position == 360) {
    position = 0;
  }
  }

  void phaseB() {
  if (digitalReadFast(H2) == HIGH) {
    if (digitalReadFast(H1) == HIGH) {
      counter++;
      position = position + k;
    } else {
      counter--;
      position = position - k;
    }
  }
  if (digitalReadFast(H2) == LOW) {
    if (digitalReadFast(H1) == LOW) {
      counter++;
      position = position + k;
    } else {
      counter--;
      position = position - k;
    }
  }
  if (position == -k) {
    position = 360-k;
  }
  if (position == 360) {
    position = 0;
  }
  }

  void phaseC() {
    counter = 0;
    counter = 0;
  }
