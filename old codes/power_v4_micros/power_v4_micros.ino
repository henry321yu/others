const int SD_CS = 10;// big c high v power w/ test pin
unsigned long currentmillis = 0;
unsigned long headmillis = 0;
unsigned long a1on, a2on, a3on, b1on, b2on, b3on, a1off, a2off, a3off, b1off, b2off, b3off = 0;
byte a1, a2, a3, b1, b2, b3, start, ssrta, ssrtb = 0;
byte charget = 90; //充電次數
long circletime = 500; // verson.1 was 150 + 50
long chargecircletime = 5; //ondelay+offdelay
long a1ondelay = 0, a1offdelay = 0;
long a2time = 450, a3time = 44; //a3time must<(circletime-a2time) or will break the circletime
long a4time = 2, delaytime = 2000, initializetime = 500; //new 2 channel big C's relay for Vinb
double utctime, TWtime[5], gpstime;
word utc = 8;
String logdata, logFileName;
int msgType, j = 0,testpint;

//power relay
//int relay[] = {16, 17, 4}; // set M pin,power pin,relay pin
int relayA[] = {6, 5, 4, 16}; // set M pin,power pin,relay pin
int relayB[] = {9, 8, 7, 17}; // // set M pin,power pin,relay pin
int r[] = {0, 1, 2, 3, 0, 1, 2, 3};
const int led = 2, buttonpin = 3, testpin = 14, testled = 15, leda = 18, ledb = 19;

void setup()
{
  circletime = circletime * 1000;
  chargecircletime = chargecircletime * 1000;
  a2time = a2time * 1000;
  a3time = a3time * 1000;
  a4time = a4time * 1000;
  delaytime = delaytime * 1000;
  initializetime = initializetime * 1000;
  a1ondelay = chargecircletime * 4 / 5;
  a1offdelay = chargecircletime / 5;
  Serial.begin(19200);
  pinMode(relayA[0], OUTPUT);
  digitalWrite(relayA[0], LOW);
  pinMode(relayA[1], OUTPUT);
  digitalWrite(relayA[1], LOW);
  pinMode(relayA[2], OUTPUT);
  digitalWrite(relayA[2], LOW);
  pinMode(relayA[3], OUTPUT);
  digitalWrite(relayA[3], LOW);
  pinMode(relayB[0], OUTPUT);
  digitalWrite(relayB[0], LOW);
  pinMode(relayB[1], OUTPUT);
  digitalWrite(relayB[1], LOW);
  pinMode(relayB[2], OUTPUT);
  digitalWrite(relayB[2], LOW);
  pinMode(relayB[3], OUTPUT);
  digitalWrite(relayB[3], LOW);
  //pinMode(buttonpin, INPUT_PULLUP);
  pinMode(testpin, INPUT);

  //pinMode(leda, OUTPUT);
  //pinMode(ledb, OUTPUT);
  //pinMode(led, OUTPUT);
  digitalWrite(led, HIGH);
  delay(2000);
  digitalWrite(led, LOW);
  delay(1000);

  headmillis = micros();
}


void loop() {
    //Serial.println(testpint);
  if (digitalRead(testpin) == LOW) {
    testpint=0;
  }
  if (digitalRead(testpin) == HIGH) {
    testpint=testpint+1;
  }
  if (testpint > 10000) {
    testrun();
    //Serial.println(F("HIGH"));
  }
  /*while (digitalRead(testpin) == LOW) {
    //testrun();
    Serial.println(F("LOW"));
    }*/
  //digitalWrite(led, LOW);
  currentmillis = micros() - headmillis;
  //Serial.println(currentmillis);Serial.print(" ");

  //text   a1=ssrA a2=powerA a3=relayA


  if (currentmillis >= initializetime && start == 0) { //initialization
  a2on = currentmillis + initializetime;
  a3on = currentmillis + initializetime;

  b2on = currentmillis + initializetime + (circletime / 2); // add B
    b3on = currentmillis + initializetime + (circletime / 2);

    start = 1;
    Serial.print(currentmillis); Serial.print(" ");
    Serial.println(F("Run"));
  }




  //A





  if (currentmillis >= a2on && a2 == 0 && a3 == 0 && start == 1) { //powerA on
  a2 = 1;
  a2on = currentmillis + circletime;
  a2off = currentmillis + a2time;
  ssrta = 0;
  //digitalWrite(leda, HIGH);
  digitalWrite(relayA[r[1]], HIGH);
    delay(1);
    Serial.print(currentmillis); Serial.print(" ");
    Serial.println(F("   a2on "));
  }

  if (a2 == 1) {
  if (ssrta < charget) {
      if (currentmillis >= a1on && a1 == 0) { //ssrA on
        a1off = currentmillis + a1ondelay;
        a1on = currentmillis + chargecircletime;
        a1 = 1;
        digitalWrite(relayA[r[0]], HIGH);
        //Serial.print(currentmillis); Serial.print(" ");
        //Serial.println(F(" a1on "));
      }
      if (currentmillis > a1off && a1 == 1) { //ssrA off
        a1 = 0;
        ssrta = ssrta + 1;
        digitalWrite(relayA[r[0]], LOW);
        //Serial.print(currentmillis); Serial.print(" ");
        //Serial.println(F(" a1off "));
      }
    }
  }

  if (currentmillis >= a2off && a2 == 1) { //powerA off %%%%
  a2 = 0;
  digitalWrite(relayA[r[0]], LOW); //incase a1 turn off
    digitalWrite(relayA[r[1]], LOW);
    delay(1);
    Serial.print(currentmillis); Serial.print(" ");
    Serial.println(F("   a2off "));
    Serial.print(F("ssrta    "));
    Serial.println(ssrta);
  }
  if (currentmillis >= a3on && a3 == 0 && a2 == 0 && start == 1) { //relayA on
  a3on = currentmillis + circletime;
  a3off = currentmillis + a3time;
  a3 = 1;
  digitalWrite(relayA[r[3]], HIGH);
    Serial.print(currentmillis); Serial.print(" ");
    Serial.println(F("     a4on "));
    delayMicroseconds(a4time);
    digitalWrite(relayA[r[2]], HIGH);
    Serial.print(currentmillis + a4time); Serial.print(" "); //手工加上a4time時間差
    Serial.println(F("       a3on "));
  }
  if (currentmillis >= a3off && a3 == 1 && start == 1) { //relayA off
  a3 = 0;
  digitalWrite(leda, LOW);
    digitalWrite(relayA[r[2]], LOW);
    Serial.print(currentmillis); Serial.print(" ");
    Serial.println(F("       a3off "));
    delayMicroseconds(a4time);
    digitalWrite(relayA[r[3]], LOW);
    Serial.print(currentmillis + a4time); Serial.print(" ");
    Serial.println(F("     a4off "));
  }





  //B




  if (currentmillis >= b2on && b2 == 0 && b3 == 0 && start == 1) { //powerB on
  b2 = 1;
  b2on = currentmillis + circletime;
  b2off = currentmillis + a2time;
  ssrtb = 0;
  digitalWrite(ledb, HIGH);
    digitalWrite(relayB[r[1]], HIGH);
    delay(1);
    Serial.print(currentmillis); Serial.print(" ");
    Serial.println(F("   b2on "));
  }

  if (b2 == 1) {
  if (ssrtb < charget) {
      if (currentmillis >= b1on && b1 == 0) { //ssrB on
        b1off = currentmillis + a1ondelay;
        b1on = currentmillis + chargecircletime;
        b1 = 1;
        digitalWrite(relayB[r[0]], HIGH);
        //Serial.print(currentmillis); Serial.print(" ");
        //Serial.println(F(" b1on "));
      }
      if (currentmillis > b1off && b1 == 1) { //ssrB off
        b1 = 0;
        ssrtb = ssrtb + 1;
        digitalWrite(relayB[r[0]], LOW);
        //Serial.print(currentmillis); Serial.print(" ");
        //Serial.println(F(" b1off "));
      }
    }
  }

  if (currentmillis >= b2off && b2 == 1) { //powerB off %%%%
  b2 = 0;
  digitalWrite(relayB[r[0]], LOW); //incase a1 turn off
    digitalWrite(relayB[r[1]], LOW);
    delay(1);
    Serial.print(currentmillis); Serial.print(" ");
    Serial.println(F("   b2off "));
    Serial.print(F("ssrtb    "));
    Serial.println(ssrtb);
  }
  if (currentmillis >= b3on && b3 == 0 && b2 == 0 && start == 1) { //relayA on
  b3on = currentmillis + circletime;
  b3off = currentmillis + a3time;
  b3 = 1;
  digitalWrite(relayB[r[3]], HIGH);
    Serial.print(currentmillis); Serial.print(" ");
    Serial.println(F("     b4on "));
    delayMicroseconds(a4time);
    digitalWrite(relayB[r[2]], HIGH);
    Serial.print(currentmillis + a4time); Serial.print(" "); //手工加上a4time時間差
    Serial.println(F("       b3on "));
  }
  if (currentmillis >= b3off && b3 == 1 && start == 1) { //relayB off
  b3 = 0;
  digitalWrite(ledb, LOW);
    digitalWrite(relayB[r[2]], LOW);
    Serial.print(currentmillis); Serial.print(" ");
    Serial.println(F("       b3off "));
    delayMicroseconds(a4time);
    digitalWrite(relayB[r[3]], LOW);
    Serial.print(currentmillis + a4time); Serial.print(" ");
    Serial.println(F("     b4off "));
  }
}







void testrun() {
  Serial.println(F("Now it's test mode"));
  digitalWrite(relayA[0], LOW);
  digitalWrite(relayA[1], LOW);
  digitalWrite(relayA[2], LOW);
  digitalWrite(relayA[3], LOW);
  digitalWrite(relayB[0], LOW);
  digitalWrite(relayB[1], LOW);
  digitalWrite(relayB[2], LOW);
  digitalWrite(relayB[3], LOW);
  a1 = 0, a2 = 0, a3 = 0, b1 = 0, b2 = 0, b3 = 0;
  start = 0;

  digitalWrite(testled, HIGH);

  currentmillis = micros() - headmillis;
  Serial.println(F("relayA"));
  Serial.print(currentmillis); Serial.print(" ");
  Serial.print(relayA[r[1]]);
  digitalWrite(relayA[r[1]], HIGH);
  Serial.println(F(" on "));
  delay(1);
  for (int i = 0; i < charget; i++) {
    digitalWrite(relayA[r[0]], HIGH);
    delayMicroseconds(a1ondelay);
    digitalWrite(relayA[r[0]], LOW);
    delayMicroseconds(a1offdelay);
  }
  currentmillis = micros() - headmillis;
  Serial.print(currentmillis); Serial.print(" ");
  Serial.print(relayA[r[1]]);
  digitalWrite(relayA[r[1]], LOW);
  Serial.println(F(" off "));
  //delayMicroseconds(delaytime);
  delay(delaytime / 1000);
  currentmillis = micros() - headmillis;
  Serial.print(currentmillis); Serial.print(" ");
  Serial.print(relayA[r[2]]);
  Serial.println(F(" on "));
  digitalWrite(relayA[r[3]], HIGH);
  delayMicroseconds(a4time);
  digitalWrite(relayA[r[2]], HIGH);
  //delayMicroseconds(a3time);
  delay(a3time / 1000);
  digitalWrite(relayA[r[2]], LOW);
  delayMicroseconds(a4time);
  digitalWrite(relayA[r[3]], LOW);
  currentmillis = micros() - headmillis;
  Serial.print(currentmillis); Serial.print(" ");
  Serial.print(relayA[r[2]]);
  Serial.println(F(" off"));
  //delayMicroseconds(delaytime);
  delay(delaytime / 1000);
  //B
  currentmillis = micros() - headmillis;
  Serial.println(F("relayB"));
  Serial.print(currentmillis); Serial.print(" ");
  Serial.print(relayB[r[1]]);
  digitalWrite(relayB[r[1]], HIGH);
  Serial.println(F(" on "));
  delay(1);
  for (int i = 0; i < charget; i++) {
    digitalWrite(relayB[r[0]], HIGH);
    delayMicroseconds(a1ondelay);
    digitalWrite(relayB[r[0]], LOW);
    delayMicroseconds(a1offdelay);
  }
  currentmillis = micros() - headmillis;
  Serial.print(currentmillis); Serial.print(" ");
  Serial.print(relayB[r[1]]);
  digitalWrite(relayB[r[1]], LOW);
  Serial.println(F(" off "));
  //delayMicroseconds(delaytime);
  delay(delaytime / 1000);
  currentmillis = micros() - headmillis;
  Serial.print(currentmillis); Serial.print(" ");
  Serial.print(relayB[r[2]]);
  Serial.println(F(" on "));
  digitalWrite(relayB[r[3]], HIGH);
  delayMicroseconds(a4time);
  digitalWrite(relayB[r[2]], HIGH);
  //delayMicroseconds(a3time);
  delay(a3time / 1000);
  digitalWrite(relayB[r[2]], LOW);
  delayMicroseconds(a4time);
  digitalWrite(relayB[r[3]], LOW);
  currentmillis = micros() - headmillis;
  Serial.print(currentmillis); Serial.print(" ");
  Serial.print(relayB[r[2]]);
  Serial.println(F(" off"));
  //delayMicroseconds(delaytime);
  delay(delaytime / 1000);
  //digitalWrite(testled, LOW);
}
