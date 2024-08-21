#include <TimeLib.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

time_t RTCTime;
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);



void setup() {
  Serial.begin(115200);
  while (!Serial && millis() < 4000 );
  Serial.println("\n" __FILE__ " " __DATE__ " " __TIME__);
  setSyncProvider(getTeensy3Time);  
  
  Serial.println(F("Wire.begin"));
  Wire.begin();  
  Serial.println(F("SSD1306 begin"));
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
  }
}

void loop() {


  Serial.print("[c] ");
  Serial.print(hour());
  Serial.print(":");
  Serial.print(minute());
  Serial.print(":");
  Serial.print(second());
  Serial.print(" ");
  Serial.print(month());
  Serial.print("/");
  Serial.print(day());
  Serial.print("/");
  Serial.print(year());
  Serial.println();

  delay(1000);
  oled(); //display oled

}

time_t getTeensy3Time() {
  return Teensy3Clock.get();
}

void oled() {
  display.clearDisplay();
  display.setTextSize(1); // Draw scale text
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("Temp:");
  display.setCursor(0, 10);
  //display.println(ntc10, 2);
  display.setCursor(32, 10);
  display.println("C");
  display.setCursor(0, 20);
  display.println("SIV:");
  display.setCursor(0, 30);
  //display.println(gt[6]);
  display.setCursor(32, 30);
  display.setCursor(0, 40);
  display.println("Accu(m):");
  display.setCursor(0, 50);
  //display.println(accu, 3);
  
  display.setCursor(60, 0);
  display.println(String(year()) + "-" +  String(month()) + "-" +  String(day()));
  display.setCursor(60, 10);
  display.println(String(hour()) + ":" +  String(minute()) + ":" +  String(second()));
  display.setCursor(60, 30);
  display.println("File:");
  display.setCursor(60, 40);
  //display.println(logFileName);
  display.setCursor(60, 50);
  //display.println(logFileName2);

  display.setCursor(60, 20);
  //display.println(f2);
  //display.setCursor(90, 20);
  //display.println("hz");

  display.setCursor(90, 20);
  //display.println(f);

  display.display();      // Show initial text
}
