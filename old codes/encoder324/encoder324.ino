#include <digitalWriteFast.h>

volatile long temp,  counter = 0; //This variable will increase or decrease depending on the rotation of encoder
int ledd;
double position;
double nposition;
double setd = 1.5;
long cc = 0;
long ccc = 0;
//int H1 = 20, H2 = 21, H3, h4;
//int H1 = 14, H2 = 16, H3 = 18, h4;
int H1 = 11, H2 = 12, H3 = 18, h4;
void setup() {
  Serial.begin (115200);

  pinModeFast(H1, INPUT_PULLUP);
  pinModeFast(H2, INPUT_PULLUP);
  pinModeFast(H3, INPUT_PULLUP);
  attachInterrupt(H1, phaseA, FALLING);
  attachInterrupt(H2, phaseB, FALLING);
  attachInterrupt(H3, phaseC, FALLING);
}

void loop() {
  if ( position != nposition) {
    Serial.print (position);
    Serial.print ('\t');
    Serial.print (cc);
    Serial.print ('\t');
    Serial.println (ccc);
    nposition = position;
  }
}

void phaseA() {
  if (digitalReadFast(H1) == HIGH) {
    if (digitalReadFast(H2) == LOW) {
      position = position + setd;
    } else {
      position = position - setd;
    }
  }
  if (digitalReadFast(H1) == LOW) {
    if (digitalReadFast(H2) == HIGH) {
      position = position + setd;
    } else {
      position = position - setd;
    }
  }
  if (position == -setd) {
    position = 360 - setd;
  }
  if (position == 360) {
    position = 0;
  }
}

void phaseB() {
  if (digitalReadFast(H2) == HIGH) {
    if (digitalReadFast(H1) == HIGH) {
      position = position + setd;
    } else {
      position = position - setd;
    }
  }
  if (digitalReadFast(H2) == LOW) {
    if (digitalReadFast(H1) == LOW) {
      position = position + setd;
    } else {
      position = position - setd;
    }
  }
  if (position == -setd) {
    position = 360 - setd;
  }
  if (position == 360) {
    position = 0;
  }
}

void phaseC() {
  Serial.println (" cycle！");
  cc = position;
  ccc += 1;
}
