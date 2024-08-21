volatile long temp,  counter = 0; //This variable will increase or decrease depending on the rotation of encoder
int ledd;
double position;
double k = 11.25;
int h1 = 20, h2 = 21, h3, h4;
void setup() {
  Serial.begin (115200);

  pinMode(h1, INPUT_PULLUP);
  pinMode(h2, INPUT_PULLUP);
  pinMode(h3, INPUT_PULLUP);
  pinMode(h4, OUTPUT);
  attachInterrupt(h1, phaseA, RISING);
  attachInterrupt(h2, phaseB, RISING);
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
  if (digitalRead(h2) == LOW) {
    counter--;
    position = position - k;
  } else {
    counter++;
    position = position + k;
  }
  if (position == -k) {
    position = 360-k;
  }
  if (position == 360) {
    position = 0;
  }
}

void phaseB() {
  if (digitalRead(h1) == LOW) {
    counter++;
    position = position + k;
  } else {
    counter--;
    position = position - k;
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
