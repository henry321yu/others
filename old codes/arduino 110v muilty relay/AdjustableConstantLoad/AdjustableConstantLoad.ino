#include <Wire.h>
#include <LiquidCrystal_I2C.h>
LiquidCrystal_I2C lcd(0x27, 16, 2);
#define clk 2
#define dt 3
#define sw 4
#define pwm 9
#define currentsense A0
#define voltagesense A1
char screen = 0;
char arrowpos = 0;
float power = 0;
float current = 0;
float curcurrent = 0;
float curpower = 0;
float curvoltage = 0;
float curcurrentraw = 0;
float zerocurrent = 514;
float curvoltraw = 0;
int counter = 0;
volatile boolean currentmode = false;
volatile boolean powermode = false;
volatile boolean TurnDetected = false;
volatile boolean up = false;
volatile boolean button = false;
const unsigned char PS_128 = (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);

byte customChar1[8] = {
  0b10000,
  0b11000,
  0b11100,
  0b11110,
  0b11110,
  0b11100,
  0b11000,
  0b10000
};

byte customChar2[8] = {
  0b00100,
  0b01110,
  0b11111,
  0b00000,
  0b00000,
  0b11111,
  0b01110,
  0b00100,
};

ISR(PCINT2_vect) {
  if (digitalRead(sw) == LOW) {
    button = true;
  }
}

void isr0 ()  {
  TurnDetected = true;
  up = (digitalRead(clk) == digitalRead(dt));
}

void setup() {
  Serial.begin(115200);
  lcd.begin();
  pinMode(sw, INPUT_PULLUP);
  pinMode(clk, INPUT);
  pinMode(dt, INPUT);
  pinMode(pwm, OUTPUT);
  pinMode(currentsense, INPUT);
  digitalWrite(pwm, LOW);
  ADCSRA &= ~PS_128;
  ADCSRA |= (1 << ADPS1) | (1 << ADPS0);
  PCICR |= 0b00000100;
  PCMSK2 |= 0b00010000;   // turn o PCINT20(D4)
  attachInterrupt(0, isr0, RISING);
  TCCR1A = 0;
  TCCR1A = (1 << COM1A1) | (1 << WGM11);
  TCCR1B = 0;
  TCCR1B = (1 << WGM13) | (1 << WGM12) | (1 << CS10);
  ICR1 = 2047;
  OCR1A = 0;
  lcd.createChar(0, customChar1);
  lcd.createChar(1, customChar2);
  lcd.clear();
  lcd.print(" ADJ CONST LOAD");
  delay(2000);
  screen0();
  lcd.setCursor(0, 0);
  lcd.write((uint8_t)0);
}

void loop() {

  if (currentmode) {
    curcurrentraw = analogRead(currentsense);
    curcurrent = ((curcurrentraw - zerocurrent) * (5000.00 / 1023.00) / 100.00);
    //Serial.print(zerocurrent);
    //Serial.print("  ");
    //Serial.print(curcurrentraw);
    //Serial.print("  ");
    //Serial.println(curcurrent);
    if (counter == 5000) {
      lcd.setCursor(4, 1);
      lcd.print(curcurrent);
      lcd.print("A ");
      counter = 0;
    }
    if (curcurrent < current) {
      OCR1A++;
    }
    else {
      OCR1A = OCR1A - 1;
    }
    counter++;
    delayMicroseconds(100);
  }

  if (powermode) {
    curcurrentraw = analogRead(currentsense);
    curcurrent = ((curcurrentraw - zerocurrent) * (5000.00 / 1023.00) / 100.00);
    curvoltraw = analogRead(voltagesense);
    curvoltage = curvoltraw * (5000.00 / 1023.00) * 7.20 /1000.00;
    curpower = curvoltage * curcurrent;
    //Serial.println(curpower);
    if (counter == 5000) {
      lcd.setCursor(4, 1);
      lcd.print(curpower);
      lcd.print("W ");
      counter = 0;
    }
    if (curpower < power) {
      OCR1A++;
    }
    else {
      OCR1A = OCR1A - 1;
    }
    counter++;
    delayMicroseconds(100);
  }

  if (TurnDetected) {
    delay(200);
    switch (screen) {
      case 0:
        switch (arrowpos) {
          case 0:
            if (!up) {
              screen0();
              lcd.setCursor(0, 1);
              lcd.write((uint8_t)0);
              arrowpos = 1;
            }
            break;
          case 1:
            if (up) {
              screen0();
              lcd.setCursor(0, 0);
              lcd.write((uint8_t)0);
              arrowpos = 0;
            }
            break;
        }
        break;
      case 1:
        switch (arrowpos) {
          case 0:
            if (!up) {
              screen1();
              lcd.setCursor(0, 1);
              lcd.write((uint8_t)0);
              arrowpos = 1;
            }
            break;
          case 1:
            if (up) {
              screen1();
              lcd.setCursor(0, 0);
              lcd.write((uint8_t)0);
              arrowpos = 0;
            }
            else {
              screen1();
              lcd.setCursor(7, 1);
              lcd.write((uint8_t)0);
              arrowpos = 2;
            }
            break;
          case 2:
            if (up) {
              screen1();
              lcd.setCursor(0, 1);
              lcd.write((uint8_t)0);
              arrowpos = 1;
            }
            break;
        }
        break;
      case 2:
        if (up) {
          power = power + 0.1;
          lcd.setCursor(7, 0);
          lcd.print(power);
          lcd.print("W");
          lcd.write((uint8_t)1);
          lcd.print("  ");
        }
        else {
          power = power - 0.1;
          if (power < 0) {
            power = 0;
          }
          lcd.setCursor(7, 0);
          lcd.print(power);
          lcd.print("W");
          lcd.write((uint8_t)1);
          lcd.print("  ");
        }
        break;
      case 4:
        switch (arrowpos) {
          case 0:
            if (!up) {
              screen4();
              lcd.setCursor(0, 1);
              lcd.write((uint8_t)0);
              arrowpos = 1;
            }
            break;
          case 1:
            if (up) {
              screen4();
              lcd.setCursor(0, 0);
              lcd.write((uint8_t)0);
              arrowpos = 0;
            }
            else {
              screen4();
              lcd.setCursor(7, 1);
              lcd.write((uint8_t)0);
              arrowpos = 2;
            }
            break;
          case 2:
            if (up) {
              screen4();
              lcd.setCursor(0, 1);
              lcd.write((uint8_t)0);
              arrowpos = 1;
            }
            break;
        }
        break;
      case 5:
        if (up) {
          current = current + 0.1;
          lcd.setCursor(9, 0);
          lcd.print(current);
          lcd.print("A");
          lcd.write((uint8_t)1);
          lcd.print(" ");
        }
        else {
          current = current - 0.1;
          if (current < 0) {
            current = 0;
          }
          lcd.setCursor(9, 0);
          lcd.print(current);
          lcd.print("A");
          lcd.write((uint8_t)1);
          lcd.print(" ");
        }
        break;
    }
    TurnDetected = false;
  }

  if (button) {
    delay(200);
    switch (screen) {
      case 0:
        if (arrowpos == 0) {
          screen = 1;
          screen1();
          lcd.setCursor(0, 0);
          lcd.write((uint8_t)0);
        }
        else {
          screen = 4;
          screen4();
          lcd.setCursor(0, 0);
          lcd.write((uint8_t)0);
        }
        break;
      case 1:
        switch (arrowpos) {
          case 0:
            screen = 2;
            screen2();
            break;
          case 1:
            powermode = true;
            screen = 3;
            screen3();
            break;
          case 2:
            screen = 0;
            screen0();
            lcd.setCursor(0, 0);
            lcd.write((uint8_t)0);
            break;
        }
        break;
      case 2:
        screen = 1;
        screen1();
        lcd.setCursor(0, 0);
        lcd.write((uint8_t)0);
        break;
      case 3:
        powermode = false;
        OCR1A = 0;
        counter = 0;
        screen = 1;
        screen1();
        lcd.setCursor(0, 0);
        lcd.write((uint8_t)0);
        break;
      case 4:
        switch (arrowpos) {
          case 0:
            screen = 5;
            screen5();
            break;
          case 1:
            screen = 6;
            screen6();
            currentmode = true;
            counter = 0;
            break;
          case 2:
            screen = 0;
            screen0();
            lcd.setCursor(0, 0);
            lcd.write((uint8_t)0);
            break;
        }
        break;
      case 5:
        screen = 4;
        screen4();
        lcd.setCursor(0, 0);
        lcd.write((uint8_t)0);
        break;
      case 6:
        screen = 4;
        screen4();
        lcd.setCursor(0, 0);
        lcd.write((uint8_t)0);
        currentmode = false;
        OCR1A = 0;
        break;
    }
    arrowpos = 0;
    button = false;
  }
}

void screen0() {
  lcd.clear();
  lcd.setCursor(1, 0);
  lcd.print("Power Mode");
  lcd.setCursor(1, 1);
  lcd.print("Current Mode");
}

void screen1() {
  lcd.clear();
  lcd.setCursor(1, 0);
  lcd.print("Power:");
  lcd.print(power);
  lcd.print("W");
  lcd.setCursor(1, 1);
  lcd.print("Start");
  lcd.setCursor(8, 1);
  lcd.print("Back");
}

void screen2() {
  lcd.clear();
  lcd.setCursor(1, 0);
  lcd.print("Power:");
  lcd.print(power);
  lcd.print("W");
  lcd.write((uint8_t)1);
}

void screen3() {
  lcd.clear();
  lcd.print("Set:");
  lcd.print(power);
  lcd.print("W");
  lcd.setCursor(0, 1);
  lcd.print("Cur:");
  lcd.print(curpower);
  lcd.print("W");
  lcd.setCursor(11, 1);
  lcd.write((uint8_t)0);
  lcd.print("STOP");
}

void screen4() {
  lcd.clear();
  lcd.setCursor(1, 0);
  lcd.print("Current:");
  lcd.print(current);
  lcd.print("A");
  lcd.setCursor(1, 1);
  lcd.print("Start");
  lcd.setCursor(8, 1);
  lcd.print("Back");
}

void screen5() {
  lcd.clear();
  lcd.setCursor(1, 0);
  lcd.print("Current:");
  lcd.print(current);
  lcd.print("A");
  lcd.write((uint8_t)1);
}

void screen6() {
  lcd.clear();
  lcd.print("Set:");
  lcd.print(current);
  lcd.print("A");
  lcd.setCursor(0, 1);
  lcd.print("Cur:");
  lcd.print(curcurrent);
  lcd.print("A");
  lcd.setCursor(11, 1);
  lcd.write((uint8_t)0);
  lcd.print("STOP");
}


