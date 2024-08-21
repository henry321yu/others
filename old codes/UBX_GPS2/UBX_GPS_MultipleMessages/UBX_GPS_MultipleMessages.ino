#include <SoftwareSerial.h>
#include <SD.h>
#include <SPI.h>

// Connect the GPS RX/TX to arduino pins 3 and 5
SoftwareSerial serial = SoftwareSerial(2, 3);
File logFile;
const int SD_CS = 10;//my
const int led = 13;
unsigned long currentmillis = 0;
unsigned long headmillis = 0;
unsigned long interval1 = 0, interval2 = 1000; //set frequency (s)
double utctime, TWtime[5], gpstime;
word utc = 8;
String logdata, logFileName;

//power relay
int relay[] = {4, 5, 6, 7}; // set relay pin
int r[] = {0, 1, 2, 0, 1, 2};
byte power;

const unsigned char UBX_HEADER[]        = { 0xB5, 0x62 };
const unsigned char NAV_POSLLH_HEADER[] = { 0x01, 0x02 };
const unsigned char NAV_STATUS_HEADER[] = { 0x01, 0x03 };

const char UBLOX_INIT[] PROGMEM = {
  // Disable NMEA
  0xB5, 0x62, 0x06, 0x01, 0x08, 0x00, 0xF0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x24, // GxGGA off
  0xB5, 0x62, 0x06, 0x01, 0x08, 0x00, 0xF0, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x2B, // GxGLL off
  0xB5, 0x62, 0x06, 0x01, 0x08, 0x00, 0xF0, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x02, 0x32, // GxGSA off
  0xB5, 0x62, 0x06, 0x01, 0x08, 0x00, 0xF0, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x03, 0x39, // GxGSV off
  0xB5, 0x62, 0x06, 0x01, 0x08, 0x00, 0xF0, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x04, 0x40, // GxRMC off
  0xB5, 0x62, 0x06, 0x01, 0x08, 0x00, 0xF0, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x05, 0x47, // GxVTG off

  // Disable UBX
  0xB5, 0x62, 0x06, 0x01, 0x08, 0x00, 0x01, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x17, 0xDC, //NAV-PVT off
  0xB5, 0x62, 0x06, 0x01, 0x08, 0x00, 0x01, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x12, 0xB9, //NAV-POSLLH off
  0xB5, 0x62, 0x06, 0x01, 0x08, 0x00, 0x01, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x13, 0xC0, //NAV-STATUS off

  // Enable UBX
  //0xB5,0x62,0x06,0x01,0x08,0x00,0x01,0x07,0x00,0x01,0x00,0x00,0x00,0x00,0x18,0xE1, //NAV-PVT on
  0xB5, 0x62, 0x06, 0x01, 0x08, 0x00, 0x01, 0x02, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x13, 0xBE, //NAV-POSLLH on
  0xB5, 0x62, 0x06, 0x01, 0x08, 0x00, 0x01, 0x03, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x14, 0xC5, //NAV-STATUS on

  // Rate
  0xB5, 0x62, 0x06, 0x08, 0x06, 0x00, 0x64, 0x00, 0x01, 0x00, 0x01, 0x00, 0x7A, 0x12, //(10Hz)
  //0xB5, 0x62, 0x06, 0x08, 0x06, 0x00, 0xC8, 0x00, 0x01, 0x00, 0x01, 0x00, 0xDE, 0x6A, //(5Hz)
  //0xB5,0x62,0x06,0x08,0x06,0x00,0xE8,0x03,0x01,0x00,0x01,0x00,0x01,0x39, //(1Hz)
};

enum _ubxMsgType {
  MT_NONE,
  MT_NAV_POSLLH,
  MT_NAV_STATUS
};

struct NAV_POSLLH {
  unsigned char cls;
  unsigned char id;
  unsigned short len;
  unsigned long iTOW;
  long lon;
  long lat;
  long height;
  long hMSL;
  unsigned long hAcc;
  unsigned long vAcc;
};

struct NAV_STATUS {
  unsigned char cls;
  unsigned char id;
  unsigned short len;
  unsigned long iTOW;
  unsigned char gpsFix;
  char flags;
  char fixStat;
  char flags2;
  unsigned long ttff;
  unsigned long msss;
};

union UBXMessage {
  NAV_POSLLH navPosllh;
  NAV_STATUS navStatus;
};

UBXMessage ubxMessage;

// The last two bytes of the message is a checksum value, used to confirm that the received payload is valid.
// The procedure used to calculate this is given as pseudo-code in the uBlox manual.
void calcChecksum(unsigned char* CK, int msgSize) {
  memset(CK, 0, 2);
  for (int i = 0; i < msgSize; i++) {
    CK[0] += ((unsigned char*)(&ubxMessage))[i];
    CK[1] += CK[0];
  }
}


// Compares the first two bytes of the ubxMessage struct with a specific message header.
// Returns true if the two bytes match.
boolean compareMsgHeader(const unsigned char* msgHeader) {
  unsigned char* ptr = (unsigned char*)(&ubxMessage);
  return ptr[0] == msgHeader[0] && ptr[1] == msgHeader[1];
}


// Reads in bytes from the GPS module and checks to see if a valid message has been constructed.
// Returns the type of the message found if successful, or MT_NONE if no message was found.
// After a successful return the contents of the ubxMessage union will be valid, for the
// message type that was found. Note that further calls to this function can invalidate the
// message content, so you must use the obtained values before calling this function again.
int processGPS() {
  static int fpos = 0;
  static unsigned char checksum[2];

  static byte currentMsgType = MT_NONE;
  static int payloadSize = sizeof(UBXMessage);

  while ( serial.available() ) {

    byte c = serial.read();
    //Serial.write(c);

    if ( fpos < 2 ) {
      // For the first two bytes we are simply looking for a match with the UBX header bytes (0xB5,0x62)
      if ( c == UBX_HEADER[fpos] )
        fpos++;
      else
        fpos = 0; // Reset to beginning state.
    }
    else {
      // If we come here then fpos >= 2, which means we have found a match with the UBX_HEADER
      // and we are now reading in the bytes that make up the payload.

      // Place the incoming byte into the ubxMessage struct. The position is fpos-2 because
      // the struct does not include the initial two-byte header (UBX_HEADER).
      if ( (fpos - 2) < payloadSize )
        ((unsigned char*)(&ubxMessage))[fpos - 2] = c;

      fpos++;

      if ( fpos == 4 ) {
        // We have just received the second byte of the message type header,
        // so now we can check to see what kind of message it is.
        if ( compareMsgHeader(NAV_POSLLH_HEADER) ) {
          currentMsgType = MT_NAV_POSLLH;
          payloadSize = sizeof(NAV_POSLLH);
        }
        else if ( compareMsgHeader(NAV_STATUS_HEADER) ) {
          currentMsgType = MT_NAV_STATUS;
          payloadSize = sizeof(NAV_STATUS);
        }
        else {
          // unknown message type, bail
          fpos = 0;
          continue;
        }
      }

      if ( fpos == (payloadSize + 2) ) {
        // All payload bytes have now been received, so we can calculate the
        // expected checksum value to compare with the next two incoming bytes.
        calcChecksum(checksum, payloadSize);
      }
      else if ( fpos == (payloadSize + 3) ) {
        // First byte after the payload, ie. first byte of the checksum.
        // Does it match the first byte of the checksum we calculated?
        if ( c != checksum[0] ) {
          // Checksum doesn't match, reset to beginning state and try again.
          fpos = 0;
        }
      }
      else if ( fpos == (payloadSize + 4) ) {
        // Second byte after the payload, ie. second byte of the checksum.
        // Does it match the second byte of the checksum we calculated?
        fpos = 0; // We will reset the state regardless of whether the checksum matches.
        if ( c == checksum[1] ) {
          // Checksum matches, we have a valid message.
          return currentMsgType;
        }
      }
      else if ( fpos > (payloadSize + 4) ) {
        // We have now read more bytes than both the expected payload and checksum
        // together, so something went wrong. Reset to beginning state and try again.
        fpos = 0;
      }
    }
  }
  return MT_NONE;
}

void setup()
{
  Serial.begin(9600);
  serial.begin(9600);

  setupgps();

  if (!SD.begin(SD_CS)) {
    Serial.println(F("Card failed, or not present"));
  }
  pinMode(SD_CS, OUTPUT);
  logFileName = nextLogFile();

  logFile = SD.open(logFileName, FILE_WRITE);
  if (logFile) {
    Serial.println(F("writing"));
  }
  // if the file didn't open, print an error:
  else {
    Serial.println(F("error opening file"));
  }

  Serial.println("initialization done.");

  delay(100);

  // relay
  pinMode(relay[0], OUTPUT);
  digitalWrite(relay[0], HIGH);


  pinMode(led, OUTPUT);
  digitalWrite(led, HIGH);
  delay(2000);
  digitalWrite(led, LOW);
  delay(1000);
  
  headmillis = millis();
}

long lat;
long lon;

int circlet = 2 * (interval2 - interval1);

void loop() { //SSR on mode
  int i = 0;
  digitalWrite(led, LOW);
  currentmillis = millis() - headmillis;
  int msgType = processGPS(); //read gps in every loop
  gpstime = ubxMessage.navPosllh.iTOW;
  transf2utc();

  if (currentmillis >= interval1 && power == 0) {
    interval1 = currentmillis + circlet;
    Serial.print(currentmillis);
    Serial.print(" ");
    Serial.print("relay ");
    Serial.print(relay[r[i]]); digitalWrite(relay[r[i]], LOW);
    Serial.print(" on ");
    power = 1;

    writeDataToCard();
  }
  if (currentmillis >= interval2 && power == 1) {
    interval2 = currentmillis + circlet;
    Serial.print(currentmillis);
    Serial.print(" ");
    Serial.print("relay ");
    Serial.print(relay[r[i]]); digitalWrite(relay[r[i]], HIGH);
    Serial.print(" off ");
    power = 0;

    writeDataToCard();
  }
}

void transf2utc() {
  double temp;

  utctime = gpstime / 1000 + (utc * 3600) - 19; //utc&leap fix

  TWtime[0] = (int)(utctime / 24 / 3600); //week
  temp = utctime - (TWtime[0] * 24 * 3600);

  TWtime[1] = (int)(temp / 3600); //hr
  temp = utctime - (TWtime[0] * 24 * 3600) - (TWtime[1] * 3600);

  TWtime[2] = (int)(temp / 60); //min
  temp = utctime - (TWtime[0] * 24 * 3600) - (TWtime[1] * 3600) - (TWtime[2] * 60);

  TWtime[3] = temp; //sec

  logdata = gpstime;
  /*logdata += ",";
    logdata += String(utctime, 3);
    logdata += ",";
    logdata += String(TWtime[0], 0);
    logdata += ",";
    logdata += String(TWtime[1], 0);
    logdata += ",";
    logdata += String(TWtime[2], 0);
    logdata += ",";
    logdata += String(TWtime[3], 3);*/

  //Serial.print(logdata);
  //Serial.print(" ");
  //Serial.println(gpstime);

  /*Serial.print(" ");
    Serial.print(" TOW "); Serial.print(gpstime);
    Serial.print(" TOWfixed "); Serial.print((utctime), 2);
    Serial.print(" week "); Serial.print(TWtime[0]);
    Serial.print(" hour "); Serial.print(TWtime[1]);
    Serial.print(" minute "); Serial.print(TWtime[2]);
    Serial.print(" second "); Serial.print((TWtime[3]), 2);
    Serial.print("\n");*/

}

void setupgps() {
  for (int i = 0; i < sizeof(UBLOX_INIT); i++) {
    serial.write( pgm_read_byte(UBLOX_INIT + i) );
    delay(5); // simulating a 38400baud pace (or less), otherwise commands are not accepted by the device.
  }
}


void writeDataToCard() {
  /*myFile = SD.open("power.CSV", FILE_WRITE);
  //String dataString = logdata + "," + power + "\n";
  //myFile.print(dataString);
  //myFile.print(logdata);
  //myFile.print(",");
  myFile.print(gpstime);
  myFile.print(",");
  myFile.print(power);
  myFile.print("\n");
  myFile.close();*/


  if (!logFile) {
    logFile = SD.open(logFileName, FILE_WRITE);
  }
  if (logFile) {
    digitalWrite(led, HIGH);
    logFile.print(gpstime);
    logFile.print(",");
    logFile.print(power);
    logFile.print(",");
    logFile.print((TWtime[3]), 5);
    logFile.print("\n");
    logFile.close();
  }
  else {
    Serial.print(F("error opening test.txt "));
  }
    Serial.print(" "); Serial.print((TWtime[3]),5);    
    Serial.println();
}

String nextLogFile(void) {
  String filename;
  int logn = 0;
  for (int i = 0; i < 999; i++) {
    // Construct a file with PREFIX[Index].SUFFIX
    filename = String("POWER") + String(logn) + String(".CSV");
    // If the file name doesn't exist, return it
    if (!SD.exists(filename))
    {
      return filename;
    }
    // Otherwise increment the index, and try again
    logn++;
  }
  return "";
}


/*originaloutpu{
  if ( msgType == MT_NAV_POSLLH ) {
    Serial.print("iTOW:");      Serial.print(ubxMessage.navPosllh.iTOW);
    Serial.print(" lat/lon: "); Serial.print(ubxMessage.navPosllh.lat/10000000.0f); Serial.print(","); Serial.print(ubxMessage.navPosllh.lon/10000000.0f);
    Serial.print(" hAcc: ");    Serial.print(ubxMessage.navPosllh.hAcc/1000.0f);
    Serial.println();
  }
  else if ( msgType == MT_NAV_STATUS ) {
    Serial.print("gpsFix:");    Serial.print(ubxMessage.navStatus.gpsFix);
    Serial.println();
  }
  }*/
