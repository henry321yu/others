const int SD_CS = 10;// big c high v power w/ test pin
unsigned long currentmillis = 0;
unsigned long headmillis = 0;
unsigned long a1on, a2on, a3on, b1on, b2on, b3on, a1off, a2off, a3off, b1off, b2off, b3off = 0;
byte a1, a2, a3, b1, b2, b3, start, ssrta, ssrtb = 0;
byte charget=100;//充電次數
int circletime = 500; // verson.1 was 150 + 50
int chargecircletime = 5; //ondelay+offdelay
int a1ondelay = 0, a1offdelay = 0;
int a2time = 450, a3time = 44; //a3time must<(circletime-a2time) or will break the circletime
int a4time = 2, delaytime = 2000; //new 2 channel big C's relay for Vinb
double utctime, TWtime[5], gpstime;
word utc = 8;
String logdata, logFileName;
int msgType, j = 0;

//power relay
//int relay[] = {16, 17, 4}; // set relay pin,power pin,M pin
int relayA[] = {6, 5, 4, 16}; // set relay pin,power pin,M pin
int relayB[] = {9, 8, 7, 17}; // set relay pin,power pin,M pin
int r[] = {0, 1, 2, 3, 0, 1, 2, 3};
const int led = 2, buttonpin = 3, testpin = 14, testled = 15, leda = 18, ledb = 19;

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

  pinMode(leda, OUTPUT);
  pinMode(ledb, OUTPUT);
  pinMode(led, OUTPUT);
  digitalWrite(led, HIGH);
  delay(2000);
  digitalWrite(led, LOW);
  delay(1000);

  headmillis = millis();
}


void loop() {
  gpstime=digitalRead(testpin);
  Serial.println(gpstime);
  delay(5);
  }
