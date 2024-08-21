#include <SoftwareSerial.h> //5 hz power w/ gps and sd
#include <SD.h>
#include <SPI.h>

// Connect the GPS RX/TX to arduino pins 3 and 5
SoftwareSerial serial = SoftwareSerial(2, 3);
File logFile;

const int SD_CS = 10;// big c high v power w/ test pin
unsigned long currentmillis = 0;
unsigned long headmillis = 0, resettime = 10000;
unsigned long a1on, a2on, a3on, b1on, b2on, b3on, a1off, a2off, a3off, b1off, b2off, b3off = 0;
byte a1, a2, a3, b1, b2, b3, start, ssrta, ssrtb, commandd = 0;
byte charget = 90; //充電次數
long circletime = 500; // verson.1 was 150 + 50
long chargecircletime = 5; //ondelay+offdelay
long a1ondelay = 0, a1offdelay = 0;
long a2time = 450, a3time = 44; //a3time must<(circletime-a2time) or will break the circletime
long a4time = 2, delaytime = 2000, initializetime = 500; //new 2 channel big C's relay for Vinb
double utctime, TWtime[5], gpstime;
word utc = 8;
String logdata, logFileName;
int msgType, j = 0, testpint;

//power relay
//int relay[] = {16, 17, 4}; // set M pin,power pin,relay pin
int relayA[] = {6, 5, 4, 16}; // set M pin,power pin,relay pin
int relayB[] = {9, 8, 7, 17}; // // set M pin,power pin,relay pin
int r[] = {0, 1, 2, 3, 0, 1, 2, 3};
const int /*led = 2, buttonpin = 3, */testpin = 14, gpspin = 15, ledGPS = 18, powerpin = 19;
const int commandpin = 1, triggerpin = 0;


//add gps and sd
byte power;
byte channel;
const unsigned char UBX_HEADER[]        = { 0xB5, 0x62 };
const unsigned char NAV_POSLLH_HEADER[] = { 0x01, 0x02 };
const unsigned char NAV_STATUS_HEADER[] = { 0x01, 0x03 };

const char UBLOX_INIT[] PROGMEM = {
  // Disable NMEA
  0xB5, 0x62, 0x06, 0x01, 0x08, 0x00, 0xF0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x24, // GxGGA off
  0xB5, 0x62, 0x06, 0x01, 0x08, 0x00, 0xF0, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x2B, // GxGLL off
  0xB5, 0x62, 0x06, 0x01, 0x08, 0x00, 0xF0, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x02, 0x32, // GxGSA off
  0xB5, 0x62, 0x06, 0x01, 0x08, 0x00, 0xF0, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x03, 0x39, // GxGSV off
  0xB5, 0x62, 0x06, 0x01, 0x08, 0x00, 0xF0, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x04, 0x40, // GxRMC off
  0xB5, 0x62, 0x06, 0x01, 0x08, 0x00, 0xF0, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x05, 0x47, // GxVTG off

  // Disable UBX
  0xB5, 0x62, 0x06, 0x01, 0x08, 0x00, 0x01, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x17, 0xDC, //NAV-PVT off
  0xB5, 0x62, 0x06, 0x01, 0x08, 0x00, 0x01, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x12, 0xB9, //NAV-POSLLH off
  0xB5, 0x62, 0x06, 0x01, 0x08, 0x00, 0x01, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x13, 0xC0, //NAV-STATUS off

  // Enable UBX
  //0xB5,0x62,0x06,0x01,0x08,0x00,0x01,0x07,0x00,0x01,0x00,0x00,0x00,0x00,0x18,0xE1, //NAV-PVT on
  0xB5, 0x62, 0x06, 0x01, 0x08, 0x00, 0x01, 0x02, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x13, 0xBE, //NAV-POSLLH on
  0xB5, 0x62, 0x06, 0x01, 0x08, 0x00, 0x01, 0x03, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x14, 0xC5, //NAV-STATUS on

  // Rate
  0xB5, 0x62, 0x06, 0x08, 0x06, 0x00, 0x64, 0x00, 0x01, 0x00, 0x01, 0x00, 0x7A, 0x12, //(10Hz)
  //0xB5, 0x62, 0x06, 0x08, 0x06, 0x00, 0xC8, 0x00, 0x01, 0x00, 0x01, 0x00, 0xDE, 0x6A, //(5Hz)
  //0xB5,0x62,0x06,0x08,0x06,0x00,0xE8,0x03,0x01,0x00,0x01,0x00,0x01,0x39, //(1Hz)
  //0xB5, 0x62, 0x06, 0x08, 0x06, 0x00, 0x64, 0x00, 0x01, 0x00, 0x01, 0x00, 0x7A, 0x12, 0xB5, 0x62, 0x06, 0x08, 0x00, 0x00, 0x0E, 0x30;//(10Hz)

  // Baud Rate
  0xB5, 0x62, 0x06, 0x00, 0x14, 0x00, 0x01, 0x00, 0x00, 0x00, 0xD0, 0x08, 0x00, 0x00, 0x00, 0xE1, 0x00, 0x00, 0x07, 0x00, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0xE2, 0xE1, 0xB5, 0x62, 0x06, 0x00, 0x01, 0x00, 0x01, 0x08, 0x22 //Baud rate 57600 00 E1 00
  //..................................................................................0x80, 0x25, 0x00, ....set baud rate(xxxxxx(HEX))  ???..............
  //byte setPortRate[] = {0xB5, 0x62, 0x06, 0x00, 0x14, 0x00, 0x01, 0x00, 0x00, 0x00, 0xD0, 0x08, 0x00, 0x00, settingsArrayPointer[3], settingsArrayPointer[4], settingsArrayPointer[5], 0x00, 0x07, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
  //const char baud57600 [] PROGMEM = "PUBX,41,1,3,3,57600,0";

  //PortRate:
  //4800   = C0 12 00
  //9600   = 80 25 00
  //19200  = 00 4B 00  **SOFTWARESERIAL LIMIT FOR ARDUINO UNO R3!**
  //38400  = 00 96 00  **SOFTWARESERIAL LIMIT FOR ARDUINO MEGA 2560!**
  //57600  = 00 E1 00
  //115200 = 00 C2 01
  //230400 = 00 84 03
};

enum _ubxMsgType {
  MT_NONE,
  MT_NAV_POSLLH,
  MT_NAV_STATUS
};

struct NAV_POSLLH {
  unsigned char cls;
  unsigned char id;
  unsigned short len;
  unsigned long iTOW;
  long lon;
  long lat;
  long height;
  long hMSL;
  unsigned long hAcc;
  unsigned long vAcc;
};

struct NAV_STATUS {
  unsigned char cls;
  unsigned char id;
  unsigned short len;
  unsigned long iTOW;
  unsigned char gpsFix;
  char flags;
  char fixStat;
  char flags2;
  unsigned long ttff;
  unsigned long msss;
};

union UBXMessage {
  NAV_POSLLH navPosllh;
  NAV_STATUS navStatus;
};

UBXMessage ubxMessage;

void setup()
{
  circletime = circletime * 1000;
  chargecircletime = chargecircletime * 1000;
  a2time = a2time * 1000;
  a3time = a3time * 1000;
  a4time = a4time * 1000;
  delaytime = delaytime * 1000;
  initializetime = initializetime * 1000;
  a1ondelay = chargecircletime * 4 / 5;
  a1offdelay = chargecircletime / 5;
  resettime = resettime * 1000;
  //Serial.begin(57600);
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
  pinMode(testpin, INPUT);
  pinMode(gpspin, INPUT);
  pinMode(ledGPS, OUTPUT);
  pinMode(powerpin, INPUT_PULLUP);
  //pinMode(commandpin, OUTPUT);           ////////////////////master command
  pinMode(triggerpin, INPUT_PULLUP);
  digitalWrite(commandpin, HIGH); 
  digitalWrite(triggerpin, HIGH);

  headmillis = micros();


  //GPS and sd
  serial.begin(9600);
  setupgps();
  serial.begin(57600);
  if (!SD.begin(SD_CS)) {
    Serial.println(F("Card failed, or not present"));
  }
  pinMode(SD_CS, OUTPUT);
  logFileName = nextLogFile();
  logFile = SD.open(logFileName, FILE_WRITE);
  if (logFile) {
    Serial.println(F("writing"));
  }
  // if the file didn't open, print an error:
  else {
    //while (1) {
    Serial.println(F("error opening file"));
    delay(200);
    //}
  }
  Serial.println(F("initialization done."));

  msgType = processGPS(); //read gps in every loop (put here due to the reading singel delay will  effect relay time
  gpstime = ubxMessage.navPosllh.iTOW;
  transf2utc();

  while (digitalRead(powerpin) == HIGH) {
    Serial.println(F(" waiting for start singel... "));
    if (digitalRead(gpspin) == HIGH) {
      digitalWrite(ledGPS, HIGH);
      Serial.println(F(" GOT gps singel！！ "));
    }
    else {
      digitalWrite(ledGPS, LOW);
      Serial.println(F(" waiting for gps singel... "));
    }
    //delay(200);
  }

  msgType = processGPS(); //read gps in every loop (put here due to the reading singel delay will  effect relay time
  gpstime = ubxMessage.navPosllh.iTOW;

  currentmillis = micros() - headmillis;
  transf2utc();
  writegpsToCard();
}


void loop() {
  if (currentmillis > resettime) {
    //digitalWrite(commandpin, LOW); ////////////////////master command
  }

  if (digitalRead(triggerpin) == LOW) {
    //resetandsave();
    commandd=1;
  }
  if (digitalRead(triggerpin) == LOW&&start == 1) {
    resetandsave();
    //delayMicroseconds(15000); //master delay?
  }

  //digitalWrite(commandpin, HIGH);  ////////////////////master command

  if (digitalRead(gpspin) == HIGH) {
    //digitalWrite(ledGPS, HIGH);   //fix beeeeeeeeeeeeee
  }
  else {
    digitalWrite(ledGPS, LOW);
  }
  //Serial.println(testpint);
  if (digitalRead(testpin) == LOW) {
    testpint = 0;
  }
  if (digitalRead(testpin) == HIGH) {
    testpint = testpint + 1;
  }
  if (testpint > 10000) {
    testrun();
    //Serial.println(F("HIGH"));
  }

  //digitalWrite(led, LOW);
  currentmillis = micros() - headmillis;
  //Serial.println(currentmillis);Serial.print(" ");

  //text   a1=ssrA a2=powerA a3=relayA


  if (currentmillis >= initializetime && start == 0&&commandd==1) { //initialization
    a2on = currentmillis + initializetime;
    a3on = currentmillis + initializetime;

    b2on = currentmillis + initializetime + (circletime / 2); // add B
    b3on = currentmillis + initializetime + (circletime / 2);

    start = 1;
    Serial.print(currentmillis); Serial.print(" ");
    Serial.println(F("Run"));
  }


  //msgType = processGPS(); //read gps in every loop (put here due to the reading singel delay will  effect relay time
  //gpstime = ubxMessage.navPosllh.iTOW;

  //A





  if (currentmillis >= a2on && a2 == 0 && a3 == 0 && start == 1) { //powerA on
    a2 = 1;
    a2on = currentmillis + circletime;
    a2off = currentmillis + a2time;
    ssrta = 0;
    //b2on = currentmillis + (circletime / 2); // add B
    digitalWrite(relayA[r[1]], HIGH);
    delay(1);
    Serial.print(currentmillis); Serial.print(" ");
    Serial.println(F("   a2on "));

  }

  if (a2 == 1) {
    if (ssrta < charget) {
      if (currentmillis >= a1on && a1 == 0) { //ssrA on
        a1off = currentmillis + a1ondelay;
        a1on = currentmillis + chargecircletime;
        a1 = 1;
        digitalWrite(relayA[r[0]], HIGH);
        //Serial.print(currentmillis); Serial.print(" ");
        //Serial.println(F(" a1on "));
      }
      if (currentmillis > a1off && a1 == 1) { //ssrA off
        a1 = 0;
        ssrta = ssrta + 1;
        digitalWrite(relayA[r[0]], LOW);
        //Serial.print(currentmillis); Serial.print(" ");
        //Serial.println(F(" a1off "));
      }
    }
  }

  if (currentmillis >= a2off && a2 == 1) { //powerA off %%%%
    a2 = 0;
    digitalWrite(relayA[r[0]], LOW); //incase a1 turn off
    digitalWrite(relayA[r[1]], LOW);
    delay(1);
    Serial.print(currentmillis); Serial.print(" ");
    Serial.println(F("   a2off "));
    Serial.print(F("ssrta    "));
    Serial.println(ssrta);
  }
  if (currentmillis >= a3on && a3 == 0 && a2 == 0 && start == 1) { //relayA on
    a3on = currentmillis + circletime;
    a3off = currentmillis + a3time;
    a3 = 1;
    digitalWrite(relayA[r[3]], HIGH);
    Serial.print(currentmillis); Serial.print(" ");
    Serial.println(F("     a4on "));
    delayMicroseconds(a4time);
    digitalWrite(relayA[r[2]], HIGH);
    Serial.print(currentmillis + a4time); Serial.print(" "); //手工加上a4time時間差
    Serial.println(F("       a3on "));
    power = 1;
    channel = 1;
    writeDataToCard();
  }
  if (currentmillis >= a3off && a3 == 1 && start == 1) { //relayA off
    a3 = 0;
    digitalWrite(relayA[r[2]], LOW);
    Serial.print(currentmillis); Serial.print(" ");
    Serial.println(F("       a3off "));
    delayMicroseconds(a4time);
    digitalWrite(relayA[r[3]], LOW);
    Serial.print(currentmillis + a4time); Serial.print(" ");
    Serial.println(F("     a4off "));
    power = 0;
    channel = 1;
    //writeDataToCard();
  }





  //B




  if (currentmillis >= b2on && b2 == 0 && b3 == 0 && start == 1) { //powerB on
    b2 = 1;
    b2on = currentmillis + circletime;
    b2off = currentmillis + a2time;
    ssrtb = 0;
    digitalWrite(relayB[r[1]], HIGH);
    delay(1);
    Serial.print(currentmillis); Serial.print(" ");
    Serial.println(F("   b2on "));
  }

  if (b2 == 1) {
    if (ssrtb < charget) {
      if (currentmillis >= b1on && b1 == 0) { //ssrB on
        b1off = currentmillis + a1ondelay;
        b1on = currentmillis + chargecircletime;
        b1 = 1;
        digitalWrite(relayB[r[0]], HIGH);
        //Serial.print(currentmillis); Serial.print(" ");
        //Serial.println(F(" b1on "));
      }
      if (currentmillis > b1off && b1 == 1) { //ssrB off
        b1 = 0;
        ssrtb = ssrtb + 1;
        digitalWrite(relayB[r[0]], LOW);
        //Serial.print(currentmillis); Serial.print(" ");
        //Serial.println(F(" b1off "));
      }
    }
  }

  if (currentmillis >= b2off && b2 == 1) { //powerB off %%%%
    b2 = 0;
    digitalWrite(relayB[r[0]], LOW); //incase a1 turn off
    digitalWrite(relayB[r[1]], LOW);
    delay(1);
    Serial.print(currentmillis); Serial.print(" ");
    Serial.println(F("   b2off "));
    Serial.print(F("ssrtb    "));
    Serial.println(ssrtb);
  }
  if (currentmillis >= b3on && b3 == 0 && b2 == 0 && start == 1) { //relayA on
    b3on = currentmillis + circletime;
    b3off = currentmillis + a3time;
    b3 = 1;
    digitalWrite(relayB[r[3]], HIGH);
    Serial.print(currentmillis); Serial.print(" ");
    Serial.println(F("     b4on "));
    delayMicroseconds(a4time);
    digitalWrite(relayB[r[2]], HIGH);
    Serial.print(currentmillis + a4time); Serial.print(" "); //手工加上a4time時間差
    Serial.println(F("       b3on "));
    power = 1;
    channel = 2;
    writeDataToCard();
  }
  if (currentmillis >= b3off && b3 == 1 && start == 1) { //relayB off
    b3 = 0;
    digitalWrite(relayB[r[2]], LOW);
    Serial.print(currentmillis); Serial.print(" ");
    Serial.println(F("       b3off "));
    delayMicroseconds(a4time);
    digitalWrite(relayB[r[3]], LOW);
    Serial.print(currentmillis + a4time); Serial.print(" ");
    Serial.println(F("     b4off "));
    power = 0;
    channel = 2;
    //writeDataToCard();
  }
}




void writegpsToCard() {
  currentmillis = micros() - headmillis;
  if (!logFile) {
    logFile = SD.open(logFileName, FILE_WRITE);
  }
  if (logFile) {
    logFile.print(currentmillis);
    logFile.print(",");
    logFile.print(gpstime);
    logFile.print(",");
    logFile.print((TWtime[1]), 0);
    logFile.print(",");
    logFile.print((TWtime[2]), 0);
    logFile.print(",");
    logFile.print((TWtime[3]), 5);
    logFile.print("\n");
    logFile.close();
  }
}


void writeDataToCard() {
  currentmillis = micros() - headmillis;
  if (!logFile) {
    logFile = SD.open(logFileName, FILE_WRITE);
  }
  if (logFile) {
    logFile.print(currentmillis);
    logFile.print(",");
    logFile.print(channel);
    logFile.print(",");
    logFile.print(power);
    logFile.print(",");
    logFile.print(ssrta);
    logFile.print(",");
    logFile.print(ssrtb);
    logFile.print("\n");
    //logFile.close();
  }
}


String nextLogFile(void) {
  String filename;
  int logn = 0;
  for (int i = 0; i < 999; i++) {
    // Construct a file with PREFIX[Index].SUFFIX
    filename = String("power") + String(logn) + String(".csv");
    // If the file name doesn't exist, return it
    if (!SD.exists(filename))
    {
      return filename;
    }
    // Otherwise increment the index, and try again
    logn++;
  }
  return "";
}


void resetandsave() { //

  digitalWrite(relayA[0], LOW);
  digitalWrite(relayA[1], LOW);
  digitalWrite(relayA[2], LOW);
  digitalWrite(relayA[3], LOW);
  digitalWrite(relayB[0], LOW);
  digitalWrite(relayB[1], LOW);
  digitalWrite(relayB[2], LOW);
  digitalWrite(relayB[3], LOW);
  a1 = 0, a2 = 0, a3 = 0, b1 = 0, b2 = 0, b3 = 0;
  start = 0;

  msgType = processGPS(); //read gps in every loop (put here due to the reading singel delay will  effect relay time
  gpstime = ubxMessage.navPosllh.iTOW;
  currentmillis = micros() - headmillis;
  transf2utc();
  writegpsToCard();

  Serial.print(currentmillis);
  Serial.println(F(" resetandsave"));
  resettime = 10000000 + currentmillis;
  digitalWrite(ledGPS, HIGH);
  delay(100);
  digitalWrite(ledGPS, LOW);
  delay(100);
  digitalWrite(ledGPS, HIGH);
  delay(100);
  digitalWrite(ledGPS, LOW);
  delay(100);
}


void transf2utc() {

  double temp = 0;

  utctime = gpstime / 1000 + (utc * 3600) - 19; //utc&leap fix

  TWtime[0] = (int)(utctime / 24 / 3600); //week
  temp = utctime - (TWtime[0] * 24 * 3600);

  TWtime[1] = (int)(temp / 3600); //hr
  temp = utctime - (TWtime[0] * 24 * 3600) - (TWtime[1] * 3600);

  TWtime[2] = (int)(temp / 60); //min
  temp = utctime - (TWtime[0] * 24 * 3600) - (TWtime[1] * 3600) - (TWtime[2] * 60);

  TWtime[3] = temp; //sec

  logdata = gpstime;
}


void testrun() {
  Serial.println(F("Now it's test mode"));
  digitalWrite(relayA[0], LOW);
  digitalWrite(relayA[1], LOW);
  digitalWrite(relayA[2], LOW);
  digitalWrite(relayA[3], LOW);
  digitalWrite(relayB[0], LOW);
  digitalWrite(relayB[1], LOW);
  digitalWrite(relayB[2], LOW);
  digitalWrite(relayB[3], LOW);
  a1 = 0, a2 = 0, a3 = 0, b1 = 0, b2 = 0, b3 = 0;
  start = 0;

  //  digitalWrite(testled, HIGH);

  currentmillis = micros() - headmillis;
  Serial.println(F("relayA"));
  Serial.print(currentmillis); Serial.print(" ");
  Serial.print(relayA[r[1]]);
  digitalWrite(relayA[r[1]], HIGH);
  Serial.println(F(" on "));
  delay(1);
  for (int i = 0; i < charget; i++) {
    digitalWrite(relayA[r[0]], HIGH);
    delayMicroseconds(a1ondelay);
    digitalWrite(relayA[r[0]], LOW);
    delayMicroseconds(a1offdelay);
  }
  currentmillis = micros() - headmillis;
  Serial.print(currentmillis); Serial.print(" ");
  Serial.print(relayA[r[1]]);
  digitalWrite(relayA[r[1]], LOW);
  Serial.println(F(" off "));
  //delayMicroseconds(delaytime);
  delay(delaytime / 1000);
  currentmillis = micros() - headmillis;
  Serial.print(currentmillis); Serial.print(" ");
  Serial.print(relayA[r[2]]);
  Serial.println(F(" on "));
  digitalWrite(relayA[r[3]], HIGH);
  delayMicroseconds(a4time);
  digitalWrite(relayA[r[2]], HIGH);
  //delayMicroseconds(a3time);
  delay(a3time / 1000);
  digitalWrite(relayA[r[2]], LOW);
  delayMicroseconds(a4time);
  digitalWrite(relayA[r[3]], LOW);
  currentmillis = micros() - headmillis;
  Serial.print(currentmillis); Serial.print(" ");
  Serial.print(relayA[r[2]]);
  Serial.println(F(" off"));
  //delayMicroseconds(delaytime);
  delay(delaytime / 1000);
  //B
  currentmillis = micros() - headmillis;
  Serial.println(F("relayB"));
  Serial.print(currentmillis); Serial.print(" ");
  Serial.print(relayB[r[1]]);
  digitalWrite(relayB[r[1]], HIGH);
  Serial.println(F(" on "));
  delay(1);
  for (int i = 0; i < charget; i++) {
    digitalWrite(relayB[r[0]], HIGH);
    delayMicroseconds(a1ondelay);
    digitalWrite(relayB[r[0]], LOW);
    delayMicroseconds(a1offdelay);
  }
  currentmillis = micros() - headmillis;
  Serial.print(currentmillis); Serial.print(" ");
  Serial.print(relayB[r[1]]);
  digitalWrite(relayB[r[1]], LOW);
  Serial.println(F(" off "));
  //delayMicroseconds(delaytime);
  delay(delaytime / 1000);
  currentmillis = micros() - headmillis;
  Serial.print(currentmillis); Serial.print(" ");
  Serial.print(relayB[r[2]]);
  Serial.println(F(" on "));
  digitalWrite(relayB[r[3]], HIGH);
  delayMicroseconds(a4time);
  digitalWrite(relayB[r[2]], HIGH);
  //delayMicroseconds(a3time);
  delay(a3time / 1000);
  digitalWrite(relayB[r[2]], LOW);
  delayMicroseconds(a4time);
  digitalWrite(relayB[r[3]], LOW);
  currentmillis = micros() - headmillis;
  Serial.print(currentmillis); Serial.print(" ");
  Serial.print(relayB[r[2]]);
  Serial.println(F(" off"));
  //delayMicroseconds(delaytime);
  delay(delaytime / 1000);
  //digitalWrite(testled, LOW);
}

void setupgps() { //setup UBLOX_INIT codes
  for (int i = 0; i < sizeof(UBLOX_INIT); i++) {
    serial.write( pgm_read_byte(UBLOX_INIT + i) );
    delay(5); // simulating a 38400baud pace (or less), otherwise commands are not accepted by the device.
  }
}


// The last two bytes of the message is a checksum value, used to confirm that the received payload is valid.
// The procedure used to calculate this is given as pseudo-code in the uBlox manual.
void calcChecksum(unsigned char* CK, int msgSize) {
  memset(CK, 0, 2);
  for (int i = 0; i < msgSize; i++) {
    CK[0] += ((unsigned char*)(&ubxMessage))[i];
    CK[1] += CK[0];
  }
}


// Compares the first two bytes of the ubxMessage struct with a specific message header.
// Returns true if the two bytes match.
boolean compareMsgHeader(const unsigned char* msgHeader) {
  unsigned char* ptr = (unsigned char*)(&ubxMessage);
  return ptr[0] == msgHeader[0] && ptr[1] == msgHeader[1];
}


// Reads in bytes from the GPS module and checks to see if a valid message has been constructed.
// Returns the type of the message found if successful, or MT_NONE if no message was found.
// After a successful return the contents of the ubxMessage union will be valid, for the
// message type that was found. Note that further calls to this function can invalidate the
// message content, so you must use the obtained values before calling this function again.
int processGPS() {
  static int fpos = 0;
  static unsigned char checksum[2];

  static byte currentMsgType = MT_NONE;
  static int payloadSize = sizeof(UBXMessage);

  while ( serial.available() ) {

    byte c = serial.read();
    //Serial.write(c);

    if ( fpos < 2 ) {
      // For the first two bytes we are simply looking for a match with the UBX header bytes (0xB5,0x62)
      if ( c == UBX_HEADER[fpos] )
        fpos++;
      else
        fpos = 0; // Reset to beginning state.
    }
    else {
      // If we come here then fpos >= 2, which means we have found a match with the UBX_HEADER
      // and we are now reading in the bytes that make up the payload.

      // Place the incoming byte into the ubxMessage struct. The position is fpos-2 because
      // the struct does not include the initial two-byte header (UBX_HEADER).
      if ( (fpos - 2) < payloadSize )
        ((unsigned char*)(&ubxMessage))[fpos - 2] = c;

      fpos++;

      if ( fpos == 4 ) {
        // We have just received the second byte of the message type header,
        // so now we can check to see what kind of message it is.
        if ( compareMsgHeader(NAV_POSLLH_HEADER) ) {
          currentMsgType = MT_NAV_POSLLH;
          payloadSize = sizeof(NAV_POSLLH);
        }
        else if ( compareMsgHeader(NAV_STATUS_HEADER) ) {
          currentMsgType = MT_NAV_STATUS;
          payloadSize = sizeof(NAV_STATUS);
        }
        else {
          // unknown message type, bail
          fpos = 0;
          continue;
        }
      }

      if ( fpos == (payloadSize + 2) ) {
        // All payload bytes have now been received, so we can calculate the
        // expected checksum value to compare with the next two incoming bytes.
        calcChecksum(checksum, payloadSize);
      }
      else if ( fpos == (payloadSize + 3) ) {
        // First byte after the payload, ie. first byte of the checksum.
        // Does it match the first byte of the checksum we calculated?
        if ( c != checksum[0] ) {
          // Checksum doesn't match, reset to beginning state and try again.
          fpos = 0;
        }
      }
      else if ( fpos == (payloadSize + 4) ) {
        // Second byte after the payload, ie. second byte of the checksum.
        // Does it match the second byte of the checksum we calculated?
        fpos = 0; // We will reset the state regardless of whether the checksum matches.
        if ( c == checksum[1] ) {
          // Checksum matches, we have a valid message.
          return currentMsgType;
        }
      }
      else if ( fpos > (payloadSize + 4) ) {
        // We have now read more bytes than both the expected payload and checksum
        // together, so something went wrong. Reset to beginning state and try again.
        fpos = 0;
      }
    }
  }
  return MT_NONE;
}
