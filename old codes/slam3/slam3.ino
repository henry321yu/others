#include <SoftwareSerial.h>

SoftwareSerial mySerial(7, 8); // RX,TX
//SoftwareSerial mySerial(9,10); // RX,TX
//SoftwareSerial mySerial(0, 1); // RX,TX
//SoftwareSerial mySerial Serial1; // RX,TX

String logdata = "";
int out = 2;
long a;
long b;
int count = 0;
long buf[100];
long readd[20];
int j = 0;
int k = 0;
int startt = 0;
int starttb = 0;
int readn = 0;
float ang[10];
float dis[10];
int qua;
int s;
unsigned long datan = 0;
int temp[10];
int flaga = 0;
int flagb = 0;
int flagc = 0;
int flagd = 0;
int flage = 0;
int flagf = 0;
int flagmix = 0;
int flags = 0;
int flagr = 0;

void setup()
{
  // Open serial communications and wait for port to open:
  Serial.begin(256000);
  mySerial.begin(256000);
  pinMode(out, OUTPUT);
  delay(100);
  Serial.println("Goodnight moon!");

  mySerial.write(0xA5);
  mySerial.write(0x40); //reset
  delay(5);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for Leonardo only
  }
  mySerial.write(0xA5);
  mySerial.write(0x21); //reset
  Serial.println("RUN BITCH");
}

void loop() // run over and over
{
  //mySerial.write(0xA5);
  //mySerial.write(0x40); //reset
  //mySerial.write(0x50); //GET_INFO        20byte A5 5A 14 00 00 00 04
  //mySerial.write(0x52); //health(status)   3byte A5 5A 3 00 00 00 06
  //mySerial.write(0x59); //GET_SAMPLERATE   4byte
  //mySerial.write(0x82); //express scan     5byte A5 5A 54 00 00 40 82
  //mySerial.write(0x20); //scan             5byte A5 5A 05 00 00 40 81
  //mySerial.write(0x21); //force scan       5byte A5 5A 05 00 00 40 81
  if (mySerial.available() <= 1) {
    mySerial.write(0xA5);
    mySerial.write(0x21);
    delay(1000);
    Serial.println("");
    Serial.println("Reboost");
    flags = 0;
    startt = 0;
  }
  while (mySerial.available()) {
    a = mySerial.read();
    datan ++;
    identity3(); //identity3 header
    Serial.print(a, HEX);  //print all read
    Serial.print(" ");
    if (startt == 1) {
      buf[count] = a;
      count += 1;
      flagr = 0;
    }
    //gotnum();
    /*if (datan > 200) {
      mySerial.write(0xA5);
      mySerial.write(0x25); //stop
      delay(2);
      }*/
  }
  if (count > 17) { //17-2個判斷 =15
    //Serial.println(" ");
    readn = (count - 10) / 5; //12-2個判斷 =10
    for (int i = 10; count > i; i++) {//取respond(7+5)後
      //Serial.print(buf[i], HEX);
      //Serial.print(" ");
      if (readn * 5 > j) {
        readd[j] = buf[i];
        //Serial.print(readd[j], HEX);
        //Serial.print(" ");
      }
      j++;
    }

    for (int i = 0; readn > i; i++) {
      ang[i] = readd[i * 5 + 1] >> 1 | readd[i * 5 + 2] << 7;
      dis[i] = readd[i * 5 + 3] | readd[i * 5 + 4] << 8;
      qua = readd[i * 5] >> 2;
      s = bitRead(readd[i * 5], 1);

      ang[i] = ang[i] / 64; //to degree
      dis[i] = dis[i] / 40; //to cm

      //Serial.println(" ");
      //Serial.print("qua = ");
      //Serial.print(qua);
      //Serial.print(" s = ");
      //Serial.print(s);
      //Serial.print("ang = ");
      //Serial.print(ang[i]);
      //Serial.print(" dis = ");
      //Serial.println(dis[i]);
    }
    //Serial.println("");
  }
  //Serial.println("");

  /*
    if (s != 0 || count < 17) {
      mySerial.write(0xA5);
      mySerial.write(0x21); //reset
      delay(1000);
    }*/

  //Serial.print("count = ");
  //Serial.println(count);

  /*if (flagr > 200) { //reset
    mySerial.write(0xA5);
    mySerial.write(0x40); //stop
    Serial.println("");
    Serial.print("flagr = ");
    Serial.println(flagr);
    flagr = 0;
    startt = 0;
    delay(4);
    }*/

  //analogWrite(out, 255);
  delay(4);
  count = 0;
  j = 0;
  startt = 0;
  starttb = 0;
  flags++;
  flagr++;
  Serial.println("");
  //Serial.print("flags = ");
  //Serial.println(flags);
}




void identity3() {
  if (temp[0] == 0xA5) {
    flaga = 1;
  }
  else {
    flaga = 0;
    temp[0] = a;
  }
  if (flaga == 1 && a == 0x5A) { //A5 5A當判別新列
    Serial.print('\n');
    Serial.print("datan = ");
    Serial.println(datan);
    Serial.print(0xA5, HEX);
    Serial.print(" ");
    datan = 0;
    startt = 1;
    flaga = 0;
    temp[0] = 0;
  }
}


void gotnum() {
  if (count > 17) { //17-2個判斷 =15
    //Serial.println(" ");
    readn = (count - 10) / 5; //12-2個判斷 =10
    for (int i = 10; count > i; i++) {//取respond(7+5)後
      //Serial.print(buf[i], HEX);
      //Serial.print(" ");
      if (readn * 5 > j) {
        readd[j] = buf[i];
        //Serial.print(readd[j], HEX);
        //Serial.print(" ");
      }
      j++;
    }

    for (int i = 0; readn > i; i++) {
      ang[i] = readd[i * 5 + 1] >> 1 | readd[i * 5 + 2] << 7;
      dis[i] = readd[i * 5 + 3] | readd[i * 5 + 4] << 8;
      qua = readd[i * 5] >> 2;
      s = bitRead(readd[i * 5], 1);

      ang[i] = ang[i] / 64; //to degree
      dis[i] = dis[i] / 40; //to cm

      //Serial.println(" ");
      //Serial.print("qua = ");
      //Serial.print(qua);
      //Serial.print(" s = ");
      //Serial.print(s);
      Serial.print("ang = ");
      Serial.print(ang[i]);
      Serial.print(" dis = ");
      Serial.println(dis[i]);
    }
    //Serial.println("");
  }
}




















void identity2() {
  if (temp[0] == 0xA5 || flaga == 1) {
    flaga = 1;
  }
  else {
    flaga = 0;
    temp[0] = a;
  }
  if (temp[1] == 0x5A || flagb == 1) {
    flagb = 1;
  }
  else {
    flagb = 0;
    temp[1] = a;
  }
  if (temp[2] == 0x05 || flagc == 1) {
    flagc = 1;
  }
  else {
    flagc = 0;
    temp[2] = a;
  }
  if (temp[3] == 0 || flagd == 1) {
    flagd = 1;
  }
  else {
    flagd = 0;
    temp[3] = a;
  }
  if (flaga == 1 && flagb == 1 && flagc == 1  &&  flagd == 1 && a == 0x00) { //A5 5A 05 00 00當判別新列
    Serial.print('\n');
    Serial.print("datan = ");
    Serial.println(datan);
    /*Serial.print(0xA5, HEX);
      Serial.print(" ");
      Serial.print(0x5A, HEX);
      Serial.print(" ");
      Serial.print(0x05, HEX);
      Serial.print(" ");
      Serial.print(0x00, HEX);
      Serial.print(" ");*/
    datan = 0;
    startt = 1;
    flaga = 0;    flagb = 0;    flagc = 0;    flagd = 0;
    temp[0] = 0;    temp[1] = 0;    temp[2] = 0;    temp[3] = 0;
  }
}


















void identity() {
  if (temp[0] == 0xA5) {
    flaga = 1;
  }
  else {
    flaga = 0;
    temp[0] = a;
  }
  if (temp[1] == 0x5A) {
    flagb = 1;
  }
  else {
    flagb = 0;
    temp[1] = a;
  }
  if (temp[2] == 0x05) {
    flagc = 1;
  }
  else {
    flagc = 0;
    temp[2] = a;
  }
  if (temp[3] == 0) {
    flagd = 1;
  }
  else {
    flagd = 0;
    temp[3] = a;
  }
  if (temp[4] == 0) {
    flage = 1;
  }
  else {
    flage = 0;
    temp[4] = a;
  }
  if (temp[5] == 0x40) {
    flagf = 1;
  }
  else {
    flagf = 0;
    temp[5] = a;
  }
  if (flaga == 1 && flagb == 1 && flagc == 1 && flagd == 1 && flage == 1 && flagf == 1 && a == 0x81) { //A5 5A 05當判別新列
    Serial.print('\n');
    Serial.print("datan = ");
    Serial.println(datan);
    Serial.print(0xA5, HEX);
    Serial.print(" ");
    Serial.print(0x5A, HEX);
    Serial.print(" ");
    Serial.print(0x05, HEX);
    Serial.print(" ");
    Serial.print(0x00, HEX);
    Serial.print(" ");
    Serial.print(0x00, HEX);
    Serial.print(" ");
    Serial.print(0x40, HEX);
    Serial.print(" ");
    datan = 0;
    startt = 1;
  }
}


/*
  if (count < 63) {
    mySerial.write(0xA5);
    mySerial.write(0x52); //stop
    while (mySerial.available()) {
      b = mySerial.read();
      if (b == 0xA5) {
        //Serial.print('\n');
        starttb = 1;
      }
      if (starttb == 1) {
        Serial.print(b, HEX);
        Serial.print(" ");
      }
    }
  }*/
