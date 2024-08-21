#include <SoftwareSerial.h>
#include <SD.h>
#include <SPI.h>

File myFile;
SoftwareSerial GPSModule(2, 3); // RX, TX
const int SD_CS = 10;
int updates;
int pos;
int stringplace = 0;
boolean updateCount = false;
unsigned long previousMillis = 0;
const long interval = 200;
String nmea[16];
//String labels[16] {"Time: ", "Latitude: ", "Hemisphere: ", "Longitude: ", "Hemisphere: ", "Quality: ", "Number of Sata: ", "HD: ", "Altitude: "};
void setup() {

  Serial.begin(9600);

  GPSModule.begin(9600);

  delay(2000);

  if (!SD.begin(SD_CS)) {
    Serial.println("initialization failed!");
    return;
  }
  pinMode(SD_CS, OUTPUT);

  Serial.println("initialization done.");

  delay(100);
}
//eg3. $GPGGA,hhmmss.ss,llll.ll,a,yyyyy.yy,a,x,xx,x.x,x.x,M,x.x,M,x.x,xxxx*hh
//1    = UTC of Position
//2    = Latitude
//3    = N or S
//4    = Longitude
//5    = E or W
//6    = GPS quality indicator (0=invalid; 1=GPS fix; 2=Diff. GPS fix)
//7    = Number of satellites in use [not those in view]
//8    = Horizontal dilution of position
//9    = Antenna altitude above/below mean sea level (geoid)
//10   = Meters  (Antenna height unit)
//11   = Geoidal separation (Diff. between WGS-84 earth ellipsoid and
//       mean sea level.  -=geoid is below WGS-84 ellipsoid)
//12   = Meters  (Units of geoidal separation)
//13   = Age in seconds since last update from diff. reference station
//14   = Diff. reference station ID#
//15   = Checksum
//
//eg4. $GPRMC,hhmmss.ss,A,llll.ll,a,yyyyy.yy,a,x.x,x.x,ddmmyy,x.x,a*hh
//1    = UTC of position fix
//2    = Data status (V=navigation receiver warning)
//3    = Latitude of fix
//4    = N or S
//5    = Longitude of fix
//6    = E or W
//7    = Speed over ground in knots
//8    = Track made good in degrees True
//9    = UT date
//10   = Magnetic variation degrees (Easterly var. subtracts from true course)
//11   = E or W
//12   = Checksum

void loop() {

  unsigned long currentMillis = millis();

  if (currentMillis - previousMillis >= interval) {
    // save the last time you blinked the LED
    previousMillis = currentMillis;
    Serial.println(previousMillis);
      writeDataToCard();

    // if the LED is off turn it on and vice-versa:
   /* if (updateCount == false) { // ?????????/
      updateCount = true;
    } else {
      updateCount = false;
    }*/
    
    Serial.flush();   // ?????????/
    GPSModule.flush();   // ?????????/
    
    while (GPSModule.available() > 0)
    {
      GPSModule.read();
    }
    if (GPSModule.find("$GPGGA,")) {

      String tempMsg = GPSModule.readStringUntil('\n');

      //for (int i = 0; i < tempMsg.length(); i++) {
      for (int i = 0; i < 10; i++) {
        if (tempMsg.substring(i, i + 1) == ",") {
          nmea[pos] = tempMsg.substring(stringplace, i);
          stringplace = i + 1;
          pos++; //1~12 (data numbers)
          
          //Serial.println(stringplace); //each data substring head(i=end) :t=0:9
          //Serial.println(nmea[pos]);
          //Serial.println(i);
        }
      }
      Serial.println(tempMsg);
    }
  }
  stringplace = 0;
  pos = 0;
}


void writeDataToCard() {

  myFile = SD.open("TEST2.CSV", FILE_WRITE);
  Serial.print(" Time: ");
  Serial.print(nmea[0]);/*
  Serial.print(" Lat: ");
  Serial.print(nmea[1]);
  Serial.print(" Lng: ");
  Serial.print(nmea[3]);
  Serial.print(" Fix: ");
  Serial.print(nmea[5]);
  Serial.print(" Sats: ");
  Serial.print(nmea[6]);
  Serial.print(" Alt: ");
  Serial.print(nmea[8]);*/
  Serial.println(" ");
  String dataString = "";
  dataString += nmea[0];/*
  dataString += ",";
  dataString += nmea[1];
  dataString += ",";
  dataString += nmea[3];
  dataString += ",";
  dataString += nmea[5];
  dataString += ",";
  dataString += nmea[6];
  dataString += ",";
  dataString += nmea[8];*/
  dataString += "\n";
  myFile.print(dataString);
  myFile.close();
}
