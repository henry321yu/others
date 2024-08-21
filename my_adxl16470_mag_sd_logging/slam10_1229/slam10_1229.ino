#include <SoftwareSerial.h>

SoftwareSerial mySerial(7, 8); // RX,TX
SoftwareSerial HC12(15, 14); // RX,TX

String logdata = "";
int out = 2;
long a;
long b;
unsigned long count = 0;
long buf[10];
int j = 0;
int k = 0;
int i = 0;
int startt = 0;
int starttb = 0;
int readn = 0;
float ang[10];
float dis[10];
float ango[50];
float diso[50];
int quao[50];
int16_t qua;
int16_t s;
int16_t rs;
int16_t c;
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
int waitpoint = 500;
unsigned long times[3];
int tags;
double f[50];
double timee;
double timeeo[50];
unsigned long kk;
int setpin = 9;
String flydata;

void setup()
{
  // Open serial communications and wait for port to open:
  Serial.begin(115200);
  mySerial.begin(256000);
  pinMode(out, OUTPUT);
  delay(100);

  Serial.println(F("HC12.reset"));
  pinMode(setpin, OUTPUT); digitalWrite(setpin, LOW); // for reset
  HC12.begin(9600);
  delay(100);
  HC12.print("AT+B115200");
  delay(100);
  Serial.println(F("HC12.begin and set"));
  HC12.begin(115200);
  delay(100);
  HC12.print("AT+B115200");
  delay(100);
  HC12.print("AT+C127"); //127 for imu、gps //117 for mag sensor
  delay(100);
  HC12.print("AT+P8");
  delay(100);
  digitalWrite(setpin, HIGH);
  Serial.println(F("HC12.set"));
  while (HC12.available()) {
    Serial.write(HC12.read());
  }

  mySerial.write(0xA5);
  mySerial.write(0x40); //reset
  delay(5);
  Serial.println("Running");
  times[2] = millis();
  analogWrite(out, 255);
}

void loop() // run over and over
{
  times[0] = millis();
  if (times[0] - times[1] >= waitpoint) {
    times[1] = times[0] + waitpoint;
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
  }
  while (mySerial.available()) {
    times[0] = millis();
    times[1] = times[0] + waitpoint;
    a = mySerial.read();
    identity3(); //identity3 header
    //Serial.print(a, HEX);  //print all read
    //Serial.print(" ");     //print all read
    if (startt == 1) {
      gotnum5();
      count ++;
    }
    datan ++;
  }
  //delay(1);
  delayMicroseconds(150);
  flaga = 0;
  //Serial.println("");       //print all read
}

void gotnum5() {
  if (i == 0) {
    buf[i] = a;
    s = bitRead(buf[0], 0);
    rs = bitRead(buf[0], 1);
    qua = buf[0] >> 2;
    //if (buf[0] != 0x3E) { // 濾除式
    //if (s==1||s == rs || qua < 1) { // 濾除式
    if (s == 1 || s == rs) { // 濾除式
      i = -1;
      tags = 0;
    }
  }
  if (i == 1) {
    buf[i] = a;
    c = bitRead(buf[1], 0);
    if (c != 1) { // 濾除式
      i = -1;
      tags = 0;
    }
  }
  if (i == 2)
    buf[i] = a;
  if (i == 3)
    buf[i] = a;
  if (i == 4) {
    buf[i] = a;

    ang[0] = buf[1] >> 1 | buf[2] << 7;
    dis[0] = buf[3] | buf[4] << 8;
    ang[1] = ang[0] / 64; //to degree
    dis[1] = dis[0] / 40; //to cm
    qua = buf[0] >> 2;
    s = bitRead(buf[0], 0);
    rs = bitRead(buf[0], 1);
    c = bitRead(buf[1], 0);

    ango[tags] = ang[1];
    diso[tags] = dis[1];
    quao[tags] = qua;
    times[0] = millis();
    timee = times[0] * 0.001;
    timeeo[tags] = timee;
    f[tags] = kk / timeeo[tags];

    if (tags >= 12) { //連續符合資格數、輸出數
      for (int j = 1; j < tags; j++) {

        kk ++;
        /*Serial.print(timeeo[j], 3);
          Serial.print('\t');
          Serial.print(ango[j], 4);
          Serial.print('\t');
          Serial.print(diso[j], 4);
          Serial.print('\t');
          //Serial.print(f[j], 2);
          //Serial.println('\t');

          Serial.print("9999");// fly ask for it
          Serial.print('\t');// fly ask for it*/

        //flydata = String(timeeo[j], 3) + '\t' + String(ango[j], 4) + '\t' + String(diso[j], 4) + '\t' + String(quao[j]) + '\t' + String(f[j], 2) + '\t' + '\n';
        flydata = String(timeeo[j], 3) + '\t' + String(ango[j], 4) + '\t' + String(diso[j], 4) + '\t' + String(quao[j]) + '\t' + String(f[j], 2) + '\t' + String("9999") + '\t';
        Serial.print(flydata);
        //HC12.println(flydata);
        /*
                  Serial.print(ango[j], 4);
                  Serial.print('\t');
                  Serial.print(diso[j], 4);
                  Serial.println('\t');*/

        ango[j] = 0;
        diso[j] = 0;
        //delay(100);
      }
      tags = 0;
    }
    tags ++;
    /*
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

    i = -1;
    buf[0] = 0;
    buf[1] = 0;
    buf[2] = 0;
    buf[3] = 0;
    buf[4] = 0;
  }
  i++;/*
  Serial.print("i = ");
  Serial.print(i);
  Serial.print(" tags = ");
  Serial.println(tags);*/
}






















void gotnum4() {
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
    ang[i] = ang[i] / 64; //to degree
    dis[i] = dis[i] / 40; //to cm
    qua = buf[0] >> 2;
    s = bitRead(buf[0], 0);
    rs = bitRead(buf[0], 1);
    c = bitRead(buf[1], 0);

    if (c == 1 && s != rs) { // 判別式
      ango[tags] = ang[i];
      diso[tags] = dis[i];
      times[0] = millis();
      timee = times[0] * 0.001;
      timeeo[tags] = timee;
      f[tags] = kk / timeeo[tags];

      if (tags == 20) { //連續符合資格數、輸出數

        /*
                Serial.print(timeeo[tags], 4);
                Serial.print('\t');
                //Serial.print("ang = ");
                Serial.print(ango[tags], 4);
                Serial.print('\t');
                //Serial.print("dis = ");
                Serial.print(diso[tags], 4);
                Serial.print('\t');
                //Serial.print("f = ");
                Serial.println(f[tags], 2);
        */


        for (int j = 0; j < tags; j++) {
          kk ++;

          /*
                    if (ang[j] >= 360 || dis[j] == 0) {
                      ;
                    }
                    else {*/

          Serial.print(timeeo[j], 3);
          Serial.print('\t');
          Serial.print(ango[j], 4);
          Serial.print('\t');
          Serial.print(diso[j], 4);
          Serial.print('\t');
          Serial.println(f[j], 2);

          flydata = String(timeeo[j], 3) + "," + String(ango[j], 4) + "," + String(diso[j], 4) + "," + String(f[tags], 2);
          //HC12.println(flydata);

          ango[j] = 0;
          diso[j] = 0;
          //delay(100);
          //}
        }
        tags = 0;
      }
      tags ++;
    }
    else {
      for (int j = 0; j < tags; j++) {
        ango[j] = 0;
        diso[j] = 0;
      }
      tags = 0;
    }

    Serial.print(buf[0], HEX);
    Serial.print(" ");
    Serial.print(buf[1], HEX);
    Serial.print(" ");
    Serial.print(buf[2], HEX);
    Serial.print(" ");
    Serial.print(buf[3], HEX);
    Serial.print(" ");
    Serial.print(buf[4], HEX);
    Serial.print(" ");


    i = -1;
    buf[0] = 0;
    buf[1] = 0;
    buf[2] = 0;
    buf[3] = 0;
    buf[4] = 0;
  }
  i++;
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
