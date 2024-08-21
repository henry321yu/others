//power test (power plus relay)

int t1 = 50; //set delay(on) time
int t2 = 150; //set delay(off) time
int relay[] = {4, 5, 6, 7}; // set relay pin
//int ssr = 3; // set ssr pin
char text, mode;
int r[] = {0, 1, 2, 0, 1, 2};

void setup() {
  Serial.begin(9600);//turn the switch OFF,cause its low trigger
  pinMode(relay[0], OUTPUT);
  /*pinMode(relay[1], OUTPUT);
  pinMode(relay[2], OUTPUT);
  pinMode(relay[3], OUTPUT);*/
  digitalWrite(relay[0], HIGH);
  /*digitalWrite(relay[1], HIGH);
  digitalWrite(relay[2], HIGH);
  digitalWrite(relay[3], HIGH);/*
  pinMode(ssr, OUTPUT);
  digitalWrite(ssr, LOW);*/
  mode = 'a';
}
void loop() {
  /*if (Serial.available() > 0) {
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
    if (mode == 'a')*/
  ON();
}

void ON() { //SSR on mode
  /*for (int i = 1; i > 3; i++) {
    Serial.print("relay "); Serial.print(relay[i]);
    Serial.print(" on");
    digitalWrite(relay[i], LOW);
    delay(t1);


    Serial.print("relay "); Serial.print(relay[r[i]]);
    Serial.print(" on");
    digitalWrite(relay[r[i + 1]], LOW);
    delay(t1);
    Serial.print("relay "); Serial.print(relay[r[i + 1]]);
    Serial.print(" on");
    digitalWrite(relay[r[i + 2]], LOW);
    delay(t1);

    Serial.print("relay "); Serial.print(relay[r[i]]);
    Serial.println(" off");
    digitalWrite(relay[r[i + 1]], HIGH);
    Serial.print("relay "); Serial.print(relay[r[i + 1]]);
    Serial.println(" off");
    digitalWrite(relay[r[i]], HIGH);
    Serial.print("relay "); Serial.print(relay[i]);
    Serial.println(" off");
    digitalWrite(relay[i], HIGH);
    delay(t2);
    }*/
    int i = 0;
  //for (int i = 0; i < 3; i++) {
    Serial.print("relay ");
    Serial.print(relay[r[i]]); digitalWrite(relay[r[i]], LOW);
    //Serial.print(relay[r[i + 1]]); digitalWrite(relay[r[i + 1]], LOW);
    //Serial.print(relay[r[i + 2]]); digitalWrite(relay[r[i + 2]], LOW);
    Serial.print(" on");
    delay(t1);

    Serial.print("relay ");
    Serial.print(relay[r[i]]); digitalWrite(relay[r[i]], HIGH);
    //Serial.print(relay[r[i + 1]]); digitalWrite(relay[r[i + 1]], HIGH);
    //Serial.print(relay[r[i + 2]]); digitalWrite(relay[r[i + 2]], HIGH);
    Serial.println(" off");
    delay(t2);
  //}*/
}
/*  void OFF() { //SSR off mode
    Serial.println("relay off");
    digitalWrite(relay, LOW);
  }*/
