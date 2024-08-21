//power test (power plus relay)

int t1 = 1400; //set delay(on) time
int t2 = 800; //set delay(off) time
int relay[] = {4, 5, 6, 7}; // set relay pin
int ssr = 3; // set ssr pin
char text, mode;
int buttonPin = 2, buttonState;    // the number of the pushbutton pin

void setup() {
  Serial.begin(9600);//turn the switch OFF,cause its low trigger
  pinMode(relay[0], OUTPUT);
  pinMode(relay[1], OUTPUT);
  pinMode(relay[2], OUTPUT);
  pinMode(relay[3], OUTPUT);
  digitalWrite(relay[0], LOW);
  digitalWrite(relay[1], LOW);
  digitalWrite(relay[2], LOW);
  digitalWrite(relay[3], LOW);
  pinMode(ssr, OUTPUT);
  digitalWrite(ssr, LOW);
  mode = 'a';
  pinMode(buttonPin, INPUT);
}
void loop() {
  buttonState = digitalRead(buttonPin);
  Serial.println(buttonState);
  if (buttonState == HIGH) {
    mode = 'b';
  }
  if (Serial.available() > 0) {
    text = Serial.read();
    //Serial.println(text);
    if (text == 'a') {
      mode = text;
      //Serial.println("SSR on");
      //digitalWrite(ssr, HIGH);
    }
    if (text == 'b') {
      mode = text;
      //Serial.println("SSR on");
      //digitalWrite(ssr, HIGH);
    }
    if (text == 'c') {
      mode = text;
      //Serial.println("SSR Off");
      //digitalWrite(ssr, LOW);
      OFF();
    }
  }
  if (mode == 'a') {
    CH1(); //4
    CH2(); //5 bin
    Serial.println("a mode");
  }
  if (mode == 'b') {
    CH3();
    Serial.println("b mode");
  }
}

void CH1() { //channle1 mode
  Serial.print("relay "); Serial.print(relay[0]);
  Serial.print(" on");
  digitalWrite(relay[0], HIGH);
  delay(t1);
  Serial.println(" off");
  digitalWrite(relay[0], LOW);
  delay(t2);
}
void CH2() { //channle12 mode
  Serial.print("relay "); Serial.print(relay[1]);
  Serial.println(" on");
  digitalWrite(relay[1], HIGH);
  delay(500);
  for (int i = 0; i < 3; i++) {
    Serial.println(" bin off");
    digitalWrite(relay[1], LOW);
    delay(20);
    Serial.println(" bin on");
    digitalWrite(relay[1], HIGH);
    delay(180);
  }
  delay(300);
  Serial.println("relay off");
  digitalWrite(relay[1], LOW);
  delay(t2);
}
void CH3() { //channle3 mode
  Serial.print("relay "); Serial.print(relay[0]); Serial.print(relay[1]);
  Serial.print(" on");
  digitalWrite(relay[0], HIGH);
  digitalWrite(relay[1], HIGH);
  delay(t1);
  Serial.println(" off");
  digitalWrite(relay[0], LOW);
  digitalWrite(relay[1], LOW);
  delay(t2);
}
void OFF() { //SSR off mode
  Serial.println("relay off");
  digitalWrite(relay, LOW);
}
