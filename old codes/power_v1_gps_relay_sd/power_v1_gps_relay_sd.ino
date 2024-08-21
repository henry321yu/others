#include <SoftwareSerial.h>
#include <SD.h>
#include <SPI.h>

//GPS SD save

File myFile;
SoftwareSerial GPSModule(2, 3); // RX, TX
const int SD_CS = 10;
int updates;
int pos;
int stringplace = 0;
boolean updateCount = false;
unsigned long currentmillis = 0;
unsigned long headmillis = 0;
unsigned long interval1 = 0,interval2 = 1000;
String nmea[16];
//String labels[16] {"Time: ", "Latitude: ", "Hemisphere: ", "Longitude: ", "Hemisphere: ", "Quality: ", "Number of Sata: ", "HD: ", "Altitude: "};


//power relay
int relay[] = {4, 5, 6, 7}; // set relay pin
char text, mode;
int r[] = {0, 1, 2, 0, 1, 2};
byte power;



void setup() {
  // gps sd
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

  // relay
  pinMode(relay[0], OUTPUT);
  digitalWrite(relay[0], HIGH);


  mode = 'a';
  delay(100);
  headmillis = millis();
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
  ON();
}


void writeDataToCard() {

  myFile = SD.open("power.CSV", FILE_WRITE);
  Serial.print(" Time: ");
  Serial.print(nmea[0]);
  Serial.print(" Power: ");
  Serial.print(power);
  Serial.println(" ");
  String dataString = "";  
  dataString += "Time: ";
  dataString += ",";
  dataString += nmea[0];
  dataString += ",";
  dataString += "Power: ";
  dataString += ",";
  dataString += power;
  dataString += "\n";
  myFile.print(dataString);
  myFile.close();
}

void ON() { //SSR on mode
  int i = 0;
  currentmillis = millis()-headmillis;

  if (currentmillis >= interval1 && power==0) {
    interval1 = currentmillis+2000;
    Serial.println(currentmillis);
    Serial.print("relay ");
    Serial.print(relay[r[i]]); digitalWrite(relay[r[i]], LOW);
    Serial.println(" on");
    power=1;

    readandsavegps();
  }
  if (currentmillis >= interval2 && power==1) {
    interval2 = currentmillis+2000;
    Serial.println(currentmillis);
    Serial.print("relay ");
    Serial.print(relay[r[i]]); digitalWrite(relay[r[i]], HIGH);
    Serial.println(" off");
    power=0;

    readandsavegps();
  }
}


void readandsavegps() {
  //Serial.flush();   // ?????????/
  //GPSModule.flush();   // ?????????/

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
      }
    }
    //Serial.println(tempMsg);
    writeDataToCard();
  }
  stringplace = 0;
  pos = 0;
}
