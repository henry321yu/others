const int SD_CS = 10;//my
const int led = 13;
unsigned long currentmillis = 0;
unsigned long headmillis = 0;
unsigned long interval1 = 0, interval2 = 0, interval3 = 0;
int offtime = 100;
int circlet = 100 + 100; // verson.1 was 150 + 50
double utctime, TWtime[5], gpstime;
word utc = 8;
String logdata, logFileName;
int msgType;

int j=0;

//power relay
int relay[] = {6, 5, 4}; // set relay pin
int r[] = {0, 1, 2, 0, 1, 2};
byte power;
int buttonpin = 9;


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
      Serial.print(relay[r[2]]); digitalWrite(relay[r[2]], LOW);
      Serial.print(F(" off "));
      delay(1);
      Serial.print(relay[r[1]]); digitalWrite(relay[r[1]], HIGH);
      Serial.println(F(" on "));

for(unsigned int i=0;i<5;i++){
  //currentmillis = millis() - headmillis;
      //Serial.println(currentmillis);
      //Serial.print(F(" relay "));
      //Serial.print(relay[r[0]]); 
      digitalWrite(relay[r[0]], HIGH);
      //Serial.print(F(" on "));
      //Serial.print(relay[r[0]]); 
      delay(100);
      digitalWrite(relay[r[0]], LOW);
      //Serial.println(F(" off "));
      delay(100);
    }
  currentmillis = millis() - headmillis;
      Serial.print(currentmillis);
      Serial.print(F(" relay "));
      Serial.print(relay[r[1]]); digitalWrite(relay[r[1]], LOW);
      Serial.print(F(" off "));
      delay(1);
      Serial.print(relay[r[2]]); digitalWrite(relay[r[2]], HIGH);
      Serial.println(F(" on "));
      digitalWrite(led, HIGH);
      delay(50);
      /*j++;
    while(j==3){
  digitalWrite(relay[0], LOW);
  digitalWrite(relay[1], HIGH);
  digitalWrite(relay[2], LOW);}*/
}
