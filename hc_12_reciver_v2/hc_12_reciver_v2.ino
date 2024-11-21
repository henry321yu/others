#include <SoftwareSerial.h>

//SoftwareSerial HC12(7, 8); int setpin = 9; //old box
SoftwareSerial HC12(21, 20); int setpin = 22; //small box
//SoftwareSerial HC12(13, 12);int setpin = 11; //pico

String temp;
String logdata = "";
float data[15];
int i = 0; int j = 0; int k = 0;
int tag[20];
char buf[300];
String logdataa, accdata, gyrdata, magdata;

void setup()
{
  // Open serial communications and wait for port to open:
  Serial.begin(115200);

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
  HC12.print("AT+C127"); //127 for imu //117 for mag sensor //107 for gps//097 for rtk // 087 for rasp power
  delay(100);
  HC12.print("AT+P8");
  delay(100);
  digitalWrite(setpin, HIGH);
  Serial.println(F("HC12.set"));
  while (HC12.available()) {
    Serial.write(HC12.read());
  }
  Serial.println("done initial");
  HC12.println("done initialize");
}

void loop() // run over and over
{
  while (HC12.available()) {
    getdata();
  }

//  while (HC12.available()) {
//    Serial.write(HC12.read());
//  }
//  while (Serial.available()) {
//    HC12.write(Serial.read());
//  }
  delay(1);
}

void getdata() {
  char inChar = HC12.read();
  logdata += (char)inChar;
  if (inChar == '\n') {
    //Serial.print("logdata ");
    //Serial.print(logdata);

    logdata.toCharArray(buf, logdata.length());
    for (i = 0; i < logdata.length(); i++) {
      if (buf[i] == ',') {
        tag[0] = 0;
        tag[j + 1] = i + 1; //跳過", "
        j++;
      }
    }
    for (k = 0; k < j; k++) {//將字串以,分割並分別存入data
      temp = logdata.substring(tag[k], tag[k + 1]);
      data[k] = temp.toFloat(); //限制5位數 !!!
      //Serial.println(temp);
      //Serial.println(data[k], 7);
      if (k == j - 1) {
        String temp = logdata.substring(tag[k + 1]);
        data[k + 1] = temp.toFloat();
        //Serial.println(data[k + 1], 7);
      }
    }

    logdataa += data[0]; //time
    logdataa += ",";
    logdataa += data[1]; //ax
    logdataa += ",";
    logdataa += data[2]; //ay
    logdataa += ",";
    logdataa += data[3]; //az
    logdataa += ",";
    logdataa += data[4]; //gx
    logdataa += ",";
    logdataa += data[5]; //gy
    logdataa += ",";
    logdataa += data[6]; //gz
    logdataa += ",";
    logdataa += data[7]; //mx
    logdataa += ",";
    logdataa += data[8]; //my
    logdataa += ",";
    logdataa += data[9]; //mz
    
    double ms;
    ms = sqrt(data[7] * data[7] + data[8] * data[8] + data[9] * data[9]);

//    accdata = String(data[1],7) + "," + String(data[2],3) + "," + String(data[3],3);
//    gyrdata = String(data[4],6) + "," + String(data[5],6) + "," + String(data[6],6);
//    magdata = String(data[7],5) + "," + String(data[8],3) + "," + String(data[9],3);
    magdata = String(data[7],5) + "," + String(data[8],3) + "," + String(data[9],3) + "," + String(ms,3);

    //Serial.print(logdata);
    //Serial.println(logdataa);
    
    //Serial.println(accdata);
    //Serial.println(gyrdata);
    Serial.println(magdata);
    logdataa = "";
    logdata = "";
    i = 0; j = 0; k = 0;
  }
}
