#include <SoftwareSerial.h>

SoftwareSerial mySerial(7, 8); // RX,TX
SoftwareSerial HC12(15, 14); // RX,TX

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
double timee;
unsigned long kk;
int setpin = 9;
String flydata;

void setup()
{
  // Open serial communications and wait for port to open:
  Serial.begin(115200);
  mySerial.begin(256000);
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
  mySerial.write(0xA5);
  mySerial.write(0x21);
  Serial.println("Running");
  times[2] = millis();
  analogWrite(out, 255);
}

void loop() // run over and over
{
  times[0] = millis();
  if (times[0] - times[1] >= waitpoint) {
    times[1] = times[1] + waitpoint;
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
      //Serial.print(a, HEX);  //print all read
      //Serial.print(" ");
      if (startt == 1) {
        gotnum4();
        count ++;
      }
      datan ++;
    }
    //Serial.print("count = ");
    //Serial.println(count);
    //Serial.println(""); //print all read
    delay(4);
    flaga = 0;
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

        if (tags >= 12) {
          tags = 0;
          kk += 1;
          timee = (times[0] - times[2]) * 0.001;
          f = kk / timee;
          Serial.print(timee, 3);
          Serial.print('\t');
          //Serial.print("ang = ");
          Serial.print(ang[i], 4);
          Serial.print('\t');
          //Serial.print("dis = ");
          Serial.print(dis[i], 4);
          Serial.print('\t');
          //Serial.print("f = ");
          Serial.println(f, 2);
          flydata = String(timee, 3) + "," + String(ang[i], 4) + "," + String(dis[i], 4) + "," + String(f, 2);
          HC12.println(flydata);

          //Serial.print(ang[i], 4);          Serial.print(",");          Serial.println(dis[i], 4);

          //Serial.println(ang[i], 4);
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
