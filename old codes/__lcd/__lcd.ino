#include <Wire.h>
#include <LiquidCrystal_PCF8574.h>
#include <digitalWriteFast.h>

int ledd;
double position, temp, temp2;
int H1 = 20, H2 = 21;//, H3, H4;
int resetp = 17;
//double setd = 0.75;//1p 角度 degree
double setd = 0.00087222;//1p 輪子圓周 cm
String halldata;
double cf = 100; //輪子圓周 cm
double distence = 0; //距離 m

LiquidCrystal_PCF8574 lcd(0x27);

void setup() {
  lcd.begin(16, 2); // 初始化LCD

  lcd.setBacklight(255);
  lcd.clear();
  lcd.setCursor(0, 0);  //設定游標位置 (字,行)
  lcd.print("position(meter):");
  lcd.setCursor(0, 1);
  lcd.print("0.0000");
  lcd.setCursor(6, 1);
  lcd.print(" m");

  Serial.begin (115200);

  pinModeFast(13, OUTPUT);
  pinModeFast(H1, INPUT_PULLUP);
  pinModeFast(H2, INPUT_PULLUP);
  pinModeFast(resetp, INPUT_PULLUP);
  attachInterrupt(H1, phaseA, CHANGE);
  attachInterrupt(H2, phaseB, CHANGE);
  attachInterrupt(resetp, resett, FALLING);
}

void loop() {
if ( position != temp ) {
  halldata = String(position, 4);
  Serial.println(halldata);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("position(meter):");
  lcd.setCursor(0, 1);
  lcd.print(halldata);
  lcd.setCursor(16, 1);
  lcd.print(" m");

  temp = position;
  ledd = map(position, 0, 360, 0, 255);
  analogWrite(13, ledd);
}


/*
  if ( distence != temp ) {
    halldata = String(distence);
    Serial.print("distence :");
    Serial.println(halldata);
    Serial.print("position :");
    Serial.println(position);
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("position :");
    lcd.setCursor(0, 1);
    lcd.print(halldata);
    lcd.setCursor(7, 1);
    lcd.print("meter");

    temp = distence;
    ledd = map(position, 0, 360, 0, 255);
    analogWrite(13, ledd);
  }*/
}

void phaseA() {
  if (digitalReadFast(H1) == HIGH) {
    if (digitalReadFast(H2) == LOW) {
      position = position + setd;
    } else {
      position = position - setd;
    }
  }
  if (digitalReadFast(H1) == LOW) {
    if (digitalReadFast(H2) == HIGH) {
      position = position + setd;
    } else {
      position = position - setd;
    }
  }

  /*
    if (position == -setd) {
    position = 360 - setd;
    }
    if (position == 360) {
    position = 0;
    }*/






  /*
    if (digitalReadFast(H1) == HIGH) {
    if (digitalReadFast(H2) == LOW) {
      position = position + setd;
    } else {
      position = position - setd;
    }
    }
    if (digitalReadFast(H1) == LOW) {
    if (digitalReadFast(H2) == HIGH) {
      position = position + setd;
    } else {
      position = position - setd;
    }
    }
    if (position == -setd) {
    position = 360 - setd;
    }
    if (position == 360) {
    position = 0;
    }


    temp2 = position / 360 * cf;
    if (position < temp) {
    distence -= temp2;
    }
    else if(position-temp==360){
    distence += temp2;
    }
    else {
    distence += temp2;
    }*/
}

void phaseB() {

  if (digitalReadFast(H2) == HIGH) {
    if (digitalReadFast(H1) == HIGH) {
      position = position + setd;
    } else {
      position = position - setd;
    }
  }
  if (digitalReadFast(H2) == LOW) {
    if (digitalReadFast(H1) == LOW) {
      position = position + setd;
    } else {
      position = position - setd;
    }
  }

  /*if (position == -setd) {
    position = 360 - setd;
    }
    if (position == 360) {
    position = 0;
    }*/




  /*
    if (digitalReadFast(H2) == HIGH) {
    if (digitalReadFast(H1) == HIGH) {
      position = position + setd;
    } else {
      position = position - setd;
    }
    }
    if (digitalReadFast(H2) == LOW) {
    if (digitalReadFast(H1) == LOW) {
      position = position + setd;
    } else {
      position = position - setd;
    }
    }
    if (position == -setd) {
    position = 360 - setd;
    }
    if (position == 360) {
    position = 0;
    }



    temp2 = position / 360 * cf;
    if (position < temp) {
    distence -= temp2;
    }
    else if(position-temp==360){
    distence += temp2;
    }
    else {
    distence += temp2;
    }*/
}

void resett() {
  position = 0;
}
