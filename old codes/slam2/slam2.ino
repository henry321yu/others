#include <SoftwareSerial.h>

SoftwareSerial mySerial(7, 8); // RX,TX
//SoftwareSerial mySerial(9,10); // RX,TX
//SoftwareSerial mySerial(0, 1); // RX,TX
//SoftwareSerial mySerial Serial1; // RX,TX

String logdata = "";
int out = 2;
long a;
int count = 0;
long buf[100];
long readd[20];
int j = 0;
int k = 0;
int startt = 1;
int readn = 0;
float ang[10];
float dis[10];

void setup()
{
  // Open serial communications and wait for port to open:
  Serial.begin(256000);
  mySerial.begin(256000);
  pinMode(out, OUTPUT);
  delay(100);
  Serial.println("Goodnight moon!");

  //mySerial.write(0xA5);
  //mySerial.write(0x40); //reset
  delay(5);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for Leonardo only
  }
  Serial.println("RUN BITCH");
}

void loop() // run over and over
{
  mySerial.write(0xA5);
  //mySerial.write(0x40); //reset
  //mySerial.write(0x50); //GET_INFO       20byte A5 5A 14 00 00 00 04
  //mySerial.write(0x52); //health(status)  3byte A5 5A 3 00 00 00 06
  //mySerial.write(0x59); //GET_SAMPLERATE  4byte
  //mySerial.write(0x20); //scan              5byte A5 5A 05 00 00 40 81
  mySerial.write(0x21); //force scan      5byte A5 5A 05 00 00 40 81
  while (mySerial.available()) {
    a = mySerial.read();
    if (a == 0xA5) {
      //Serial.print('\n');
      startt = 1;
    }
    if (startt == 1) {
      buf[count] = a;
      count += 1;
      //Serial.print(a, HEX);
      //Serial.print(" ");
      //if (count > 30) {
      //mySerial.write(0xA5);
      //mySerial.write(0x25); //stop
      //break;
      //}
    }
  }
  readn = (count - 12) / 5;
  //Serial.println(" ");
  for (int i = 12; count > i; i++) {//取respond(7+5)後
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

    ang[i] = ang[i] / 64; //to degree
    dis[i] = dis[i] / 40; //to cm

    Serial.println(" ");
    Serial.print("ang = ");
    Serial.print(ang[i]);
    Serial.print(" dis = ");
    Serial.print(dis[i]);
  }
  //Serial.println(" ");
  //Serial.print("count = ");
  //Serial.println(count);

  //mySerial.write(0xA5);
  //mySerial.write(0x25); //stop

  analogWrite(out, 255);
  delay(1000);
  count = 0;
  j = 0;
  startt = 0;
}





/*
  void loop() // run over and over
  {
  mySerial.write(0xA5);
  //mySerial.write(0x40); //reset
  //mySerial.write(0x50); //GET_INFO       20byte A5 5A 14 00 00 00 04
  //mySerial.write(0x52); //health(status)  3byte A5 5A 3 00 00 00 06
  //mySerial.write(0x59); //GET_SAMPLERATE  4byte
  //mySerial.write(0x20); //scan              5byte A5 5A 05 00 00 40 81
  mySerial.write(0x21); //force scan      5byte A5 5A 05 00 00 40 81
  //mySerial.write(0x82); //force scan      5byte A5 5A 05 00 00 40 81
  while (mySerial.available()) {
    a = mySerial.read();
    buf[count] = a;
    count += 1;
    if (a == 0xA5) {
      Serial.print('\n');
      for (int i = 7; count > i; i++) {
        //Serial.print(buf[i]);
        //Serial.print(" ");
      }
      //Serial.print('\n');
      Serial.print("count ");
      Serial.println(count);
      count = 0;
    }
    Serial.print(a, HEX);
    Serial.print(" ");
  }
  //analogWrite(out, 255);
  delay(1000);

  //Serial.println("");
  }*/


































/*
  void loop() // run over and over
  {
  if (count < 7) {
    Serial.println("gethealth");
    mySerial.write(0xA5);
    mySerial.write(0x52);
    analogWrite(out, 255);
  }
  else {
    mySerial.write(0xA5);
    //mySerial.write(0x40); //reset
    //mySerial.write(0x50); //GET_INFO       20byte A5 5A 14 00 00 00 04
    //mySerial.write(0x52); //health(status)  3byte A5 5A 3 00 00 00 06
    //mySerial.write(0x59); //GET_SAMPLERATE  4byte
    //mySerial.write(0x20); //scan              5byte A5 5A 05 00 00 40 81
    mySerial.write(0x21); //force scan      5byte A5 5A 05 00 00 40 81
  }
  delay(100);
  count = 0;
  while (mySerial.available()) {
    a = mySerial.read();
    buf[count] = (int)a;
    count += 1;
    Serial.print(a, HEX);
    Serial.print(" ");
    if (count > 7) {
      //Serial.print(a, HEX);
      //Serial.print(" ");
    }

  }
  //analogWrite(out, 255);
  delay(100);

  Serial.println("");
  for (int i = 7; count > i; i++) {
    Serial.println(buf[i]);
  }
  Serial.print("count ");
  Serial.println(count);
  }*/




/*
  void loop() // run over and over
  {
  mySerial.write(0xA5);
  //mySerial.write(0x40); //reset
  //mySerial.write(0x50); //GET_INFO 20byte
  //mySerial.write(0x52); //health(status) 3byte
  //mySerial.write(0x59); //GET_SAMPLERATE  4byte
  //mySerial.write(0x20); //scan 5byte
  mySerial.write(0x21); //force scan 5byte
  //delay(2);

  while (mySerial.available()) {
    a = mySerial.read();
    buf[count] = (int)a;
    count += 1;
    if (a == 0xA5) {
      //Serial.print('\n');
      //Serial.println(logdata);
      logdata = "";
    }
    //Serial.print(a, HEX);
    //Serial.print(" ");
    if (count > 7) {
      Serial.print(a, HEX);
      Serial.print(" ");
    }
  }
  //analogWrite(out, 255);
  delay(100);

  Serial.println("");
  for (int i = 7; count > i; i++) {
    Serial.println(buf[i]);
  }
  if (count < 7) {
    mySerial.write(0xA5);
    mySerial.write(0x52); //reset
    //analogWrite(out, 255);
  }
  Serial.print("count ");
  Serial.println(count);
  count = 0;
  }

*/



















/*
  void loop() // run over and over
  {
  mySerial.write(0xA5);
  mySerial.write(0x52); //health(status)

  if (mySerial.available()) {
    a = mySerial.read();
    if (a == 0xA5) {
      Serial.print('\n');
    }
    Serial.print(a, HEX);
    Serial.print(" ");
  }
  }*/










/*
  void loop() // run over and over
  {
  while (mySerial.available()) {
    Serial.write(mySerial.read());
    char inChar = mySerial.read();
    logdata += (char)inChar;
    //if (inChar == '\n') {
    //Serial.print(logdata);
    Serial.print(inChar);
    logdata = "";
    //}
  }
  Serial.println("running");
  delay(1);
  }*/






/*
  void setup()
  {
  // Open serial communications and wait for port to open:
  Serial.begin(256000);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for Leonardo only
  }


  //Serial.println("Goodnight moon!");

  // set the data rate for the SoftwareSerial port
  mySerial.begin(256000);
  //mySerial.println("Hello, world?");
  }

  void loop() // run over and over
  {
  if (mySerial.available())
    Serial.write(mySerial.read());
    int a =mySerial.read();
  mySerial.println(a,BIN);
  if (Serial.available()){
    //mySerial.write(Serial.read());
  }
  }*/
