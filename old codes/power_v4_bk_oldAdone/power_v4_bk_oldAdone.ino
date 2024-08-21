const int SD_CS = 10;// big c high v power w/ test pin
const int led = 13, leda = 14, ledb = 15;
unsigned long currentmillis = 0;
unsigned long headmillis = 0;
unsigned long a1on = 0, a2on = 0, a3on = 0, b1on = 0, b2on = 0, b3on = 0, a1off = 0, a2off = 0, a3off = 0, b1off = 0, b2off = 0, b3off = 0;
byte a1 = 0, a2 = 0, a3 = 0, b1 = 0, b2 = 0, b3 = 0;
int circletime = 1000; // verson.1 was 150 + 50
int chargecircletime = 5; //ondelay+offdelay
int a1ondelay = 0, a1offdelay = 0;
int a2time = 500, a3time = 400; //a3time must<(circletime-a2time) or will break the circletime
int a4time=2; //new 2 channel big C's relay for Vinb
double utctime, TWtime[5], gpstime;
word utc = 8;
String logdata, logFileName;
int msgType, j = 0;

//power relay
//int relay[] = {16, 17, 4}; // set relay pin,power pin,M pin
int relayA[] = {6, 5, 4, 16}; // set relay pin,power pin,M pin
int relayB[] = {9, 8, 7, 17}; // set relay pin,power pin,M pin
int r[] = {0, 1, 2, 3, 0, 1, 2, 3};
byte power;
int buttonpin = 10, testpin = 14;

void setup()
{
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
  pinMode(buttonpin, INPUT_PULLUP);
  pinMode(testpin, INPUT_PULLUP);

  pinMode(led, OUTPUT);
  digitalWrite(led, HIGH);
  delay(2000);
  digitalWrite(led, LOW);
  delay(1000);

  headmillis = millis();
}


void loop() {
  //digitalWrite(led, LOW);
  currentmillis = millis() - headmillis;
  //Serial.println(currentmillis);Serial.print(" ");

  //text   a1=ssrA a2=powerA a3=relayA


  if (currentmillis < 1000) { //initialization
    //a1on = currentmillis + chargecircletime;
    //a1off = a1on + a1offdelay;
    a2on = currentmillis + 500;
    Serial.print(currentmillis); Serial.print(" ");
    Serial.print("start at");
    Serial.print(" a1on ");
    Serial.print(a1on);
    Serial.print(" a1off ");
    Serial.println(a1off);
  }



  if (currentmillis >= a2on && a2 == 0 && a3 == 0) { //powerA on
    a2 = 1;
    a2on = currentmillis + circletime;
    a2off = currentmillis + a2time;
    digitalWrite(relayA[r[1]], HIGH);
    delay(1);
    Serial.print(currentmillis); Serial.print(" ");
    Serial.println(F("   a2on "));
  }

  if (a2 == 1) {
    if (currentmillis >= a1on && a1 == 0) { //ssrA on
      a1off = currentmillis + a1offdelay;
      a1on = currentmillis + chargecircletime;
      a1 = 1;
      digitalWrite(relayA[r[0]], HIGH);
      Serial.print(currentmillis); Serial.print(" ");
      Serial.println(F(" a1on "));
    }
    if (currentmillis > a1off && a1 == 1) { //ssrA off
      a1 = 0;
      digitalWrite(relayA[r[0]], LOW);
      Serial.print(currentmillis); Serial.print(" ");
      Serial.println(F(" a1off "));
    }
  }

  if (currentmillis >= a2off && a2 == 1) { //powerA off %%%%
    a2 = 0;
    digitalWrite(relayA[r[0]], LOW); //incase a1 turn off
    digitalWrite(relayA[r[1]], LOW);
    delay(1);
    Serial.print(currentmillis); Serial.print(" ");
    Serial.println(F("   a2off "));
  }



  if (currentmillis >= a3on && a3 == 0 && a2 == 0) { //relayA on
    a3on = currentmillis + circletime;
    a3off = currentmillis + a3time;
    a3 = 1;
    digitalWrite(relayA[r[2]], HIGH);
    Serial.print(currentmillis); Serial.print(" ");
    Serial.println(F("     a3on "));
  }
  if (currentmillis >= a3off && a3 == 1) { //relayA off
    a3 = 0;
    digitalWrite(relayA[r[2]], LOW);
    Serial.print(currentmillis); Serial.print(" ");
    Serial.println(F("     a3off "));
  }


  /*if (b2on == 0 && b3on == 0 && b2off == 0 && b3off == 0) { //initialization
    b2on = a2on - circletime*0.5; b3on = a3on - circletime*0.5; b2off = a2off - circletime*0.5; b3off = a3off - circletime*0.5;
    }
    if (b1on == 0 && b1off == 0) { //initialization
    b1on = ondelay + circletime*0.5; b1off = offdelay + circletime*0.5;
    }

    if (currentmillis >= b1off) { //ssrB off
    b1off = b1off + chargecircletime; digitalWrite(relayB[r[0]], LOW);
    Serial.print(currentmillis);Serial.print(" ");
    Serial.println(F(" b1off "));
    }
    if (currentmillis >= b1on) { //ssrB on
    b1on = b1on + chargecircletime; digitalWrite(relayB[r[0]], HIGH);
    Serial.print(currentmillis);Serial.print(" ");
    Serial.println(F(" b1on "));
    }


    if (currentmillis >= b2off) { //powerB off
    b2off = b2off + circletime;
    b1off = b1off + ontime;
    b1on = b1on + ontime;
    a1off = a1off + ontime; //added
    a1on = a1on + ontime;
    digitalWrite(relayB[r[0]], LOW);  //added
    digitalWrite(relayB[r[1]], LOW);
    digitalWrite(relayA[r[0]], LOW);  //added
    digitalWrite(relayA[r[1]], LOW);
    Serial.print(currentmillis);Serial.print(" ");
    Serial.println(F(" powerBoff "));
    Serial.println(F(" b2off "));
    Serial.println(F(" b1off "));
    Serial.println(F(" a1off "));
    Serial.println(F(" a2off "));
    }
    if (currentmillis >= b3on) { //relayB on
    b3on = b3on + circletime;
    digitalWrite(relayB[r[2]], HIGH);
    Serial.print(currentmillis);Serial.print(" ");
    Serial.println(F(" b3on "));
    }
    if (currentmillis >= b3off) { //relayB off
    b3off = b3off + circletime;
    digitalWrite(relayB[r[2]], LOW);
    Serial.print(currentmillis);Serial.print(" ");
    Serial.println(F(" b3off "));
    }

    if (currentmillis >= b2on) { //powerB on
    b2on = b2on + circletime;
    digitalWrite(relayB[r[1]], HIGH);
    Serial.print(currentmillis);Serial.print(" ");
    Serial.println(F(" b2on "));
    }*/



  /*if (digitalRead(testpin) == LOW) //pause a while when test pin low
    {
    delay(5000);
    digitalWrite(relay[r[2]], HIGH);
    delay(47);
    digitalWrite(relay[r[2]], LOW);
    delay(5000);
    }
    else {
    digitalWrite(relay[r[2]], HIGH);
    delay(47);
    }
    Serial.println(F(" on "));*/
}
