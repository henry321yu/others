#include <SoftwareSerial.h>
#include <SD.h>
#include <SPI.h>

File myFile;
SoftwareSerial GPSModule = SoftwareSerial(2, 3); // RX, TX
const int SD_CS = 10;
int updates;
int pos;
int stringplace = 0;
boolean updateCount = false;
unsigned long previousMillis = 0;
const long interval = 100;
String nmea[16];
//String labels[16] {"Time: ", "Latitude: ", "Hemisphere: ", "Longitude: ", "Hemisphere: ", "Quality: ", "Number of Sata: ", "HD: ", "Altitude: "};
const char UBLOX_INIT[] PROGMEM = {
  //0xB5, 0x62, 0x06, 0x08, 0x06, 0x00, 0x64, 0x00, 0x01, 0x00, 0x01, 0x00, 0x7A, 0x12, //(10Hz)
  0xB5,0x62,0x06,0x08,0x06,0x00,0xC8,0x00,0x01,0x00,0x01,0x00,0xDE,0x6A, //(5Hz)
  //0xB5,0x62,0x06,0x08,0x06,0x00,0xE8,0x03,0x01,0x00,0x01,0x00,0x01,0x39, //(1Hz)
};



void setup() {

  Serial.begin(9600);

  GPSModule.begin(9600);

  //initGPS();  //setup gps

  delay(2000);

  if (!SD.begin(SD_CS)) {
    Serial.println("initialization failed!");
    return;
  }
  pinMode(SD_CS, OUTPUT);

  Serial.println("initialization done.");

  delay(100);
}

void loop() {

  unsigned long currentMillis = millis();

  if (currentMillis - previousMillis >= interval) {
  previousMillis = currentMillis;
  //Serial.println(previousMillis);
  //writeDataToCard();

  //Serial.flush();   // ?????????
  //GPSModule.flush();   // ?????????

  while (GPSModule.available() > 0) {
    GPSModule.read();
  }
  if (GPSModule.find("$GPGGA,")) {

    String tempMsg = GPSModule.readStringUntil('\n');

    for (int i = 0; i < tempMsg.length(); i++) {
    //for (int i = 0; i < 1; i++) {
      if (tempMsg.substring(i, i + 1) == ",") {
        nmea[pos] = tempMsg.substring(stringplace, i);
        stringplace = i + 1;
        pos++; //1~12 (data numbers)
      }
    }
    Serial.println(tempMsg);
    //Serial.println(nmea[0]);
  }
  }
  stringplace = 0;
  pos = 0;
}

void writeDataToCard() {
  myFile = SD.open("TEST2.CSV", FILE_WRITE);
  Serial.print(" Time: ");
  Serial.println(nmea[0]);
  String dataString = "";
  dataString += nmea[0];
  dataString += "\n";
  myFile.print(dataString);
  myFile.close();
}

void initGPS() {
  for (int i = 0; i < sizeof(UBLOX_INIT); i++) {
    GPSModule.write(pgm_read_byte(UBLOX_INIT[+i]));
    delay(5);
  }
}



/*#define oldGPSBaud 9600
  #define newGPSBaud 115200
  char GPSlog[50], HEADERlog[80]; //adjust sizes
  char latitude_string[10], longitude_string[10], speed_string[10];
  const unsigned char UBLOX_INIT[] = {
  0xB5, 0x62, 0x06, 0x08, 0x06, 0x00, 0x64, 0x00, 0x01, 0x00, 0x01, 0x00, 0x7A, 0x12,//(10Hz)
  //0xB5, 0x62, 0x06, 0x01, 0x08, 0x00, 0xF0, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x2B, // GxGLL off
  //0xB5, 0x62, 0x06, 0x01, 0x08, 0x00, 0xF0, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x03, 0x39, // GxGSV off
  //0xB5, 0x62, 0x06, 0x01, 0x08, 0x00, 0xF0, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x05, 0x47, // GxVTG off
  //0xB5,0x62,0x06,0x01,0x08,0x00,0xF0,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x00,0x24, // GxGGA off
  //0xB5,0x62,0x06,0x01,0x08,0x00,0xF0,0x02,0x00,0x00,0x00,0x00,0x00,0x01,0x02,0x32, // GxGSA off
  };
  void setup() {
  Serial.begin(115200);
  Serial.print("size:");
  Serial.println(sizeof(UBLOX_INIT));
  initGPS();
  }
  void initGPS() {
  gps_port.begin(oldGPSBaud);
  // send configuration data in UBX protocol
  for (byte i = 0; i < sizeof(UBLOX_INIT); i++) {
    gps_port.write(UBLOX_INIT[i]);
  }

  delay (100);
  gps_port.print("$PUBX,41,1,0003,0001,115200,0*1E\r\n");
  delay (100);
  gps_port.end();
  delay (100);
  gps_port.begin(newGPSBaud);
  }
  void loop() {
  if (gps_port.available()) {
    char a = gps_port.read();
    Serial.print(a);
  }
  }*/
