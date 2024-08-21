#include <SoftwareSerial.h>

SoftwareSerial mySerial(7, 8); // RX,TX
//SoftwareSerial mySerial(9,10); // RX,TX
//SoftwareSerial mySerial(0, 1); // RX,TX
//SoftwareSerial mySerial Serial1; // RX,TX

String logdata = "";
int out = 2;
long a;
long b;
unsigned long count = 0;
long buf[100];
long readd[100];
int j = 0;
int k = 0;
int i = 0;
int startt = 0;
int starttb = 0;
int readn = 0;
float ang[10];
float dis[10];
int qua;
int s;
int rs;
int c;
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
int waitpoint = 10;
unsigned long times[3];
int tags;
double f;
unsigned long kk;

void setup()
{
  // Open serial communications and wait for port to open:
  Serial.begin(250000);
  mySerial.begin(256000);
  pinMode(out, OUTPUT);
  delay(100);
  //Serial.println("Goodnight moon!");

  mySerial.write(0xA5);
  mySerial.write(0x40); //reset
  delay(5);
  /*while (!Serial) {
    ; // wait for serial port to connect. Needed for Leonardo only
    }*/
  mySerial.write(0xA5);
  mySerial.write(0x21);
  Serial.println("RUN BITCH");
  times[2] = millis();
}

void loop() // run over and over
{
  times[0] = millis();
  if (times[0] - times[1] >= waitpoint) {
    times[1] = times[1] + waitpoint;

    //mySerial.write(0xA5);
    //mySerial.write(0x40); //reset
    //mySerial.write(0x50); //GET_INFO        20byte A5 5A 14 00 00 00 04
    //mySerial.write(0x52); //health(status)   3byte A5 5A 3 00 00 00 06
    //mySerial.write(0x59); //GET_SAMPLERATE   4byte
    //mySerial.write(0x82); //express scan     5byte A5 5A 54 00 00 40 82
    //mySerial.write(0x20); //scan             5byte A5 5A 05 00 00 40 81
    //mySerial.write(0x21); //force scan       5byte A5 5A 05 00 00 40 81
    if (mySerial.available() < 1) {
      mySerial.write(0xA5);
      mySerial.write(0x21);
      delay(1000);
      Serial.println("");
      Serial.println("Reboost");
      flags = 0;
      startt = 0;
      datan = 0;
      count = 0;
      i = 0;
    }
    while (mySerial.available()) {
      a = mySerial.read();
      identity3(); //identity3 header
      //Serial.print('\n');  //print all read
      //Serial.print(a, HEX);  //print all read
      //Serial.print(" ");
      if (startt == 1) {
        //Serial.print(a, HEX);  //print all read
        //Serial.print(" ");
        gotnum4();
        count ++;
      }
      /*if (datan > 500) {  // one round in a roll
        mySerial.write(0xA5);
        mySerial.write(0x25); //stop
        delay(2);
        }*/
      //analogWrite(out, 255);
      datan ++;
    }

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
    delay(3);
    flaga = 0;
    //Serial.println("");
    //Serial.print("flags = ");
    //Serial.println(flags);
    //Serial.println("");
    //Serial.println("loop");
  }
}

void gotnum4() {
  if (count > 10) { //17-1個判斷 =16
    if (i == 0)
      buf[i] = a;
    if (i == 1)
      buf[i] = a;
    if (i == 2)
      buf[i] = a;
    if (i == 3)
      buf[i] = a;
    if (i == 4) {
      buf[i] = a;

      ang[i] = buf[1] >> 1 | buf[2] << 7;
      dis[i] = buf[3] | buf[4] << 8;
      qua = buf[0] >> 2;
      s = bitRead(buf[0], 0);
      rs = bitRead(buf[0], 1);
      c = bitRead(buf[1], 0);
      if (c != 1 || s == rs) { // 判別位
        tags = 0;
      }
      else {
        tags += 1;
        ang[i] = ang[i] / 64; //to degree
        dis[i] = dis[i] / 40; //to cm


        if (tags > 10) {
          tags = 0;
          kk += 1;
          f = (times[0] - times[2]) * 0.001;
          f = kk / f;
          /*Serial.print("ang = ");
            Serial.print(ang[i],4);
            Serial.print('\t');
            Serial.print("dis = ");
            Serial.print(dis[i],4);
            Serial.print('\t');
            Serial.print("f = ");
            Serial.println(f);*/

          Serial.print(ang[i], 4);
          Serial.print(",");
          Serial.println(dis[i], 4);
        }
      }

      i = -1;
      buf[0] = 0;
      buf[1] = 0;
      buf[2] = 0;
      buf[3] = 0;
      buf[4] = 0;
    }
    i++;
  }
}


void gotnum3() {
  if (count > 10) { //17-1個判斷 =16
    if (i == 0)
      buf[i] = a;
    if (i == 1)
      buf[i] = a;
    if (i == 2)
      buf[i] = a;
    if (i == 3)
      buf[i] = a;
    if (i == 4) {
      buf[i] = a;

      ang[i] = buf[1] >> 1 | buf[2] << 7;
      dis[i] = buf[3] | buf[4] << 8;
      qua = buf[0] >> 2;
      s = bitRead(buf[0], 0);
      rs = bitRead(buf[0], 1);
      c = bitRead(buf[1], 0);
      if (c != 1 || s == rs) { // 判別位
      }
      else {
        ang[i] = ang[i] / 64; //to degree
        dis[i] = dis[i] / 40; //to cm

        if (dis[i] > 150 || dis[i] == 0 || ang[i] > 360) {
        }
        else {
          Serial.print("ang = ");
          Serial.print(ang[i]);
          Serial.print(" dis = ");
          Serial.println(dis[i]);
        }
      }

      i = -1;
      buf[0] = 0;
      buf[1] = 0;
      buf[2] = 0;
      buf[3] = 0;
      buf[4] = 0;
    }
    i++;
  }
}

void gotnum2() {
  if (count > 10) { //17-1個判斷 =16
    if (i == 0)
      buf[i] = a;
    if (i == 1)
      buf[i] = a;
    if (i == 2)
      buf[i] = a;
    if (i == 3)
      buf[i] = a;
    if (i == 4) {
      buf[i] = a;

      ang[i] = buf[1] >> 1 | buf[2] << 7;
      dis[i] = buf[3] | buf[4] << 8;
      qua = buf[0] >> 2;
      s = bitRead(buf[0], 0);
      rs = bitRead(buf[0], 1);
      c = bitRead(buf[1], 0);

      if (c != 1 || s == rs) { // 判別位
        /*mySerial.write(0xA5);
          mySerial.write(0x25); //stop
          delay(2);
          Serial.println("erro");

          Serial.print(buf[0], HEX);
          Serial.print(" ");
          Serial.print(buf[1], HEX);
          Serial.print(" ");
          Serial.print(buf[2], HEX);
          Serial.print(" ");
          Serial.print(buf[3], HEX);
          Serial.print(" ");
          Serial.print(buf[4], HEX);
          Serial.print(" ");*/
      }
      else {
        ang[i] = ang[i] / 64; //to degree
        dis[i] = dis[i] / 40; //to cm

        /*Serial.print(buf[0], HEX);
          Serial.print(" ");
          Serial.print(buf[1], HEX);
          Serial.print(" ");
          Serial.print(buf[2], HEX);
          Serial.print(" ");
          Serial.print(buf[3], HEX);
          Serial.print(" ");
          Serial.print(buf[4], HEX);
          Serial.print(" ");

          Serial.print(" count = ");
          Serial.print(count);
          Serial.print(" datan = ");
          Serial.print(datan);
          Serial.println("");*/

        Serial.print("ang = ");
        Serial.print(ang[i]);
        Serial.print(" dis = ");
        Serial.println(dis[i]);
        /*
                Serial.print(" s = ");
                Serial.print(s);
                Serial.print(" rs = ");
                Serial.print(rs);
                Serial.print(" c = ");
                Serial.print(c);
                Serial.print(" buf[0] = ");
                Serial.print(buf[0], HEX);
                Serial.print(" buf[1] = ");
                Serial.print(buf[1], HEX);
                Serial.print(" count = ");
                Serial.print(count);
                Serial.print(" datan = ");
                Serial.print(datan);
                Serial.println("");*/
      }

      i = -1;
      buf[0] = 0;
      buf[1] = 0;
      buf[2] = 0;
      buf[3] = 0;
      buf[4] = 0;
    }
    i++;
  }
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
    /*Serial.print('\n');
      Serial.print("datan = ");
      Serial.println(datan);
      Serial.print(0xA5, HEX);
      Serial.print(" ");*/
    datan = 0;
    startt = 1;
    flaga = 0;
    temp[0] = 0;
  }
}
