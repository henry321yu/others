#include <SoftwareSerial.h> //HC12(mcu's rx、hc-12's tx,   mcu's tx、hc-12's rx) 
//SoftwareSerial HC12(7, 8); int setpin = 9; //old box
SoftwareSerial HC12(21, 20); int setpin = 22; //small box
//SoftwareSerial HC12(13, 12);int setpin = 11; //pico

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

String temp;
String logdata = "";
//float data[18];
String data[20];  //for String data
int i = 0; int j = 0; int k = 0;
int tag[20];
char buf[300];
String logdataa;
unsigned long tagg = 1000;
int setch = 17;
int setch2 = 16;
String channelp = "127"; //imu


void setup()
{
  // Open serial communications and wait for port to open:
  Serial.begin(115200);

  Serial.println(F("SSD1306 begin"));
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
  }
  display.clearDisplay();
  display.setTextSize(1); // Draw scale text
  display.setTextColor(SSD1306_WHITE);
  delay(100);

  Serial.println(F("HC12.reset"));
  pinMode(setpin, OUTPUT); digitalWrite(setpin, LOW); // for reset
  pinMode(setch, INPUT_PULLUP);pinMode(setch2, INPUT_PULLUP);
  if (digitalRead(setch) == LOW) {
    channelp = "117"; //power_sensor
    if (digitalRead(setch) == LOW && digitalRead(setch2) == LOW) {
      channelp = "107"; //yan_gps
    }
  }
  HC12.begin(9600);
  delay(100);
  HC12.print("AT+B115200");
  delay(100);
  Serial.println(F("HC12.begin and set"));
  HC12.begin(115200);
  delay(100);
  HC12.print("AT+B115200");
  delay(100);
  HC12.print("AT+C" + channelp); //127 for imu、gps //117 for mag sensor
  delay(100);
  HC12.print("AT+P8");
  delay(100);
  digitalWrite(setpin, HIGH);
  Serial.println(F("HC12.set"));
  while (HC12.available()) {
    Serial.write(HC12.read());
  }
  Serial.println(F("done initial"));
  HC12.println("done initialize");

  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("initialized");
  display.setCursor(0, 50);
  display.println("channel:");
  display.setCursor(64, 50);
  display.println(channelp);
  display.display();
  delay(1000);
}

void loop() // run over and over
{
  while (HC12.available()) {
    //Serial.write(HC12.read());
    getdata();
  }
  delay(1);
}
void getdata() {
  char inChar = HC12.read();
  logdata += (char)inChar;
  if (inChar == '\n') {
    if (logdata.length() > 150) {
      logdata = "";
    }
    //Serial.print(logdata);
    logdata.toCharArray(buf, logdata.length());
    for (i = 0; i < logdata.length(); i++) {
      if (buf[i] == ',') {
        tag[0] = 0;
        tag[j + 1] = i; //跳過", "
        j++;
      }
    }
    for (k = 0; k < j; k++) {//將字串以,分割並分別存入data
      if (k == 0) {                                       //for", "(String data)
        temp = logdata.substring(tag[k], tag[k + 1]);
      }
      else {
        temp = logdata.substring(tag[k] + 1, tag[k + 1]);
      }
      //data[k] = temp.toFloat(); //限制5位數 !!!
      data[k] = temp;                                     //for String data
      logdataa += temp;
      logdataa += ",";

      if (k == j - 1) {
        temp = logdata.substring(tag[k + 1] + 1); //for fly ","(String data)
        //data[k + 1] = temp.toFloat();
        data[k + 1] = temp;                                 //for String data
        logdataa += temp;
      }
    }
    //Serial.print(logdata);
    Serial.print(logdataa);
    logdataa = "";
    logdata = "";
    i = 0; j = 0; k = 0;
    if (millis() > tagg) {
      tagg = tagg + 100;
      oled(); //display oled
    }
    data[0] = ""; data[1] = ""; data[2] = ""; data[3] = ""; data[4] = ""; data[5] = ""; data[6] = ""; //for String data
    data[7] = ""; data[8] = ""; data[9] = ""; data[10] = ""; data[11] = ""; data[12] = ""; data[13] = "";
  }
}
void oled() {
  display.clearDisplay();
  display.setCursor(0, 0);
  display.println(data[0]);
  display.setCursor(0, 9);
  display.println(data[1]);
  display.setCursor(0, 18);
  display.println(data[2]);
  display.setCursor(0, 27);
  display.println(data[3]);
  display.setCursor(0, 36);
  display.println(data[4]);
  display.setCursor(0, 45);
  display.println(data[5]);
  display.setCursor(0, 54);
  display.println(data[6]);

  display.setCursor(64, 0);
  display.println(data[7]);
  display.setCursor(64, 9);
  display.println(data[8]);
  display.setCursor(64, 18);
  display.println(data[9]);
  display.setCursor(64, 27);
  display.println(data[10]);
  display.setCursor(64, 36);
  display.println(data[11]);
  display.setCursor(64, 45);
  display.println(data[12]);
  display.setCursor(64, 54);
  display.println(data[13]);

  display.display();
}
