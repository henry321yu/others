//power test (power plus relay)

int t1 = 3000; //set delay(on) time
int t2 = 1500;
int t3 = 750;
int t4 = 750;
int relay[] = {4, 5, 6, 7}; // set relay pin
int ssr = 3; // set ssr pin
char text, mode;

void setup() {
  Serial.begin(9600);//turn the switch OFF,cause its low trigger
  pinMode(relay[0], OUTPUT);
  pinMode(relay[1], OUTPUT);
  pinMode(relay[2], OUTPUT);
  pinMode(relay[3], OUTPUT);
  digitalWrite(relay[0], HIGH);
  digitalWrite(relay[1], HIGH);
  digitalWrite(relay[2], HIGH);
  digitalWrite(relay[3], HIGH);
  pinMode(ssr, OUTPUT);
  digitalWrite(ssr, LOW);
}
void loop() {
  if (Serial.available() > 0) {
    text = Serial.read();
    //Serial.println(text);
    if (text == 'a') {
      mode = text;
      Serial.println("SSR on");
      digitalWrite(ssr, HIGH);
    }
    if (text == 'b') {
      mode = text;
      Serial.println("SSR Off");
      digitalWrite(ssr, LOW);
      //OFF();
    }
  }
  //if (mode == 'a')
    //ON();
}

void ON() { //SSR on mode
  /*for (int i = 0; i <= 3; i++){
    Serial.print("relay "); Serial.print(relay[i]);
    Serial.print(" on");
    digitalWrite(relay[i], LOW);
    delay(t);
    Serial.println(" off");
    digitalWrite(relay[i], HIGH);
    delay(t);
    }*/
    Serial.print("relay "); Serial.print(relay[0]);
    Serial.print(" on");
    digitalWrite(relay[0], LOW);
    delay(t1);
    Serial.println(" off");
    digitalWrite(relay[0], HIGH);
    delay(500);
    Serial.print("relay "); Serial.print(relay[1]);
    Serial.print(" on");
    digitalWrite(relay[1], LOW);
    delay(t2);
    Serial.println(" off");
    digitalWrite(relay[1], HIGH);
    delay(500);
    Serial.print("relay "); Serial.print(relay[2]);
    Serial.print(" on");
    digitalWrite(relay[2], LOW);
    delay(t3);
    Serial.println(" off");
    digitalWrite(relay[2], HIGH);
    delay(500);
    Serial.print("relay "); Serial.print(relay[3]);
    Serial.print(" on");
    digitalWrite(relay[3], LOW);
    delay(t4);
    Serial.println(" off");
    digitalWrite(relay[3], HIGH);
    delay(500);
    }
  void OFF() { //SSR off mode
    Serial.println("relay off");
    digitalWrite(relay, HIGH);
  }
