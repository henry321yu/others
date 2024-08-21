const int commandpin = 1, triggerpin = 0, led = 18;

unsigned long currentmillis = 0;
unsigned long headmillis = 0, resettime = 30000000;


void setup() {
  pinMode(commandpin, OUTPUT);
  pinMode(triggerpin, INPUT);
  pinMode(led, OUTPUT);
  //digitalWrite(commandpin, LOW);
  //digitalWrite(triggerpin, LOW);
  digitalWrite(commandpin, HIGH);
  digitalWrite(triggerpin, HIGH);
}

void loop() {
  /*if (currentmillis > resettime) {
    digitalWrite(commandpin, HIGH); ////////////////////master command   //都要low trigger 並在30s後
  }

  if (digitalRead(triggerpin) == HIGH) {
    resetandsave();
  }
  delay(100);
  digitalWrite(commandpin, LOW);    ////////////////////master command*/

  if (currentmillis > resettime) {
    digitalWrite(commandpin, LOW); ////////////////////master command
    }

    if (digitalRead(triggerpin) == LOW) {
    resetandsave();
    }
    //delay(100);
    digitalWrite(commandpin, HIGH);    ////////////////////master command



  currentmillis = micros();
}



void resetandsave() { //
  currentmillis = micros();

  //Serial.print(currentmillis);
  //Serial.println(F(" resetandsave"));
  resettime = 1000000 + currentmillis;
  digitalWrite(led, HIGH);
  delay(500);
  digitalWrite(led, LOW);
  delay(100);
}
