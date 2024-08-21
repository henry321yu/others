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
int a2time = 1, a3time = 80, delaytime = 1000, a4time = 3;

//power relay
//int relay[] = {16, 17, 4}; // set relay pin,power pin,M pin
//int relay[] = {6, 5, 4}; // set relay pin,power pin,M pin
int relay[] = {6, 5, 4 , 9, 8, 7, 16, 17}; // set relay pin,power pin,M pin
int r[] = {0, 1, 2, 3, 4, 5, 6, 7};
byte power;
int testpin = 14;


void setup()
{
  Serial.begin(9600);

  pinMode(relay[0], OUTPUT);
  digitalWrite(relay[0], LOW);
  pinMode(relay[1], OUTPUT);
  digitalWrite(relay[1], LOW);
  pinMode(relay[2], OUTPUT);
  digitalWrite(relay[2], LOW);
  pinMode(relay[3], OUTPUT);
  digitalWrite(relay[3], LOW);
  pinMode(relay[4], OUTPUT);
  digitalWrite(relay[4], LOW);
  pinMode(relay[5], OUTPUT);
  digitalWrite(relay[5], LOW);
  pinMode(relay[6], OUTPUT);
  digitalWrite(relay[6], LOW);
  pinMode(relay[7], OUTPUT);
  digitalWrite(relay[7], LOW);
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
  Serial.print(relay[r[1]]);
  digitalWrite(relay[r[1]], HIGH);
  Serial.println(F(" on "));
  delay(a2time);
  for (int i = 0; i < 100; i++) {
    digitalWrite(relay[r[0]], HIGH);
    delay(4);
    digitalWrite(relay[r[0]], LOW);
    delay(1);
  }
  currentmillis = millis() - headmillis;
  Serial.print(currentmillis);
  Serial.print(F(" relay "));
  Serial.print(relay[r[1]]);
  digitalWrite(relay[r[1]], LOW);
  Serial.print(F(" off "));
  delay(a2time);
  Serial.print(relay[r[2]]);
  digitalWrite(relay[r[6]], HIGH);
  delay(a4time);
  digitalWrite(relay[r[2]], HIGH);
  delay(a3time);
  digitalWrite(relay[r[2]], LOW);
  delay(a4time);
  digitalWrite(relay[r[6]], LOW);
  delay(delaytime);
  if (digitalRead(testpin) == LOW) //pause a while when test pin low
  {
    Serial.print(currentmillis);
    Serial.print(F(" relay "));
    Serial.print(relay[r[4]]);
    digitalWrite(relay[r[4]], HIGH);
    Serial.println(F(" on "));
    delay(a2time);
    for (int i = 0; i < 100; i++) {
      digitalWrite(relay[r[3]], HIGH);
      delay(4);
      digitalWrite(relay[r[3]], LOW);
      delay(1);
    }
    currentmillis = millis() - headmillis;
    Serial.print(currentmillis);
    Serial.print(F(" relay "));
    Serial.print(relay[r[4]]);
    digitalWrite(relay[r[4]], LOW);
    Serial.print(F(" off "));
    delay(a2time);
    Serial.print(relay[r[5]]);
    digitalWrite(relay[r[7]], HIGH);
    delay(a4time);
    digitalWrite(relay[r[5]], HIGH);
    delay(a3time);
    digitalWrite(relay[r[5]], LOW);
    delay(a4time);
    digitalWrite(relay[r[7]], LOW);
  delay(delaytime);
  }
}
