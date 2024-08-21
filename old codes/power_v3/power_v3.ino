const int SD_CS = 10;// big c high v power w/ test pin
const int led = 13;
unsigned long currentmillis = 0;
unsigned long headmillis = 0;
unsigned long interval1 = 0, interval2 = 0, interval3 = 0;
int offtime = 100;
int circlet = 100 + 100; // verson.1 was 150 + 50
double utctime, TWtime[5], gpstime;
word utc = 8;
String logdata, logFileName;
int msgType, j = 0;

//power relay
//int relay[] = {16, 17, 4}; // set relay pin,power pin,M pin
//int relay[] = {6, 5, 4}; // set relay pin,power pin,M pin
int relay[] = {9,8,7}; // set relay pin,power pin,M pin
int r[] = {0, 1, 2, 0, 1, 2};
byte power;
int buttonpin = 9, testpin = 14;


void setup()
{
  Serial.begin(9600);

  pinMode(relay[0], OUTPUT);
  digitalWrite(relay[0], LOW);
  pinMode(relay[1], OUTPUT);
  digitalWrite(relay[1], LOW);
  pinMode(relay[2], OUTPUT);
  digitalWrite(relay[2], LOW);
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
  digitalWrite(led, LOW);
  currentmillis = millis() - headmillis;
  Serial.print(currentmillis);
  Serial.print(F(" relay "));
  Serial.print(relay[r[2]]);
  digitalWrite(relay[r[2]], LOW);
  Serial.print(F(" off "));
  delay(1);
  Serial.print(relay[r[1]]);
  digitalWrite(relay[r[1]], HIGH);
  Serial.println(F(" on "));
  delay(1);
  for (int i = 0; i < 100; i++) {
    //currentmillis = millis() - headmillis;
    //Serial.print(currentmillis);
    //Serial.print(F(" relay "));
    //Serial.print(relay[r[0]]);
    digitalWrite(relay[r[0]], HIGH);
    //Serial.print(F(" on "));
    //delayMicroseconds(1800);
    delay(4);
    //delayMicroseconds(250);
    //Serial.print(relay[r[0]]);
    digitalWrite(relay[r[0]], LOW);
    //Serial.println(F(" off "));
    //delayMicroseconds(700);
    delay(1);
    //delayMicroseconds(150);
  }
  currentmillis = millis() - headmillis;
  Serial.print(currentmillis);
  Serial.print(F(" relay "));
  Serial.print(relay[r[1]]);
  digitalWrite(relay[r[1]], LOW);
  Serial.print(F(" off "));
  delay(1);
  Serial.print(relay[r[2]]);
  if (digitalRead(testpin) == LOW) //pause a while when test pin low
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
  Serial.println(F(" on "));
}
