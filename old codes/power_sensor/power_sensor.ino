#include <SoftwareSerial.h> //5 hz power w/ gps and sd
#include <SD.h>
#include <SPI.h>

// Connect the GPS RX/TX to arduino pins 3 and 5
SoftwareSerial serial = SoftwareSerial(2, 3);
File logFile;

const int SD_CS = 10;// big c high v power w/ test pin
unsigned long currentmillis = 0;
unsigned long headmillis = 0, resettime = 10000;
//unsigned long a1on, a2on, a3on, b1on, b2on, b3on, a1off, a2off, a3off, b1off, b2off, b3off = 0;
//byte a1, a2, a3, b1, b2, b3, start, ssrta, ssrtb, commandd = 0;
//byte charget = 90; //充電次數
//long circletime = 500; // verson.1 was 150 + 50
//long chargecircletime = 5; //ondelay+offdelay
//long a1ondelay = 0, a1offdelay = 0;
//long a2time = 450, a3time = 44; //a3time must<(circletime-a2time) or will break the circletime
//long a4time = 2, delaytime = 2000, initializetime = 500; //new 2 channel big C's relay for Vinb
double utctime, TWtime[5], gpstime;
word utc = 8;
String logdata, logFileName;
int msgType, j = 0, testpint;
const int H1 = A7; //hall need wire
int hall, hallo=0, hb = 7;
unsigned long i = 0;
double f;
int maxsize = 209715200;

/*//power relay
  //int relay[] = {16, 17, 4}; // set M pin,power pin,relay pin
  int relayA[] = {6, 5, 4, 16}; // set M pin,power pin,relay pin
  int relayB[] = {9, 8, 7, 17}; // // set M pin,power pin,relay pin
  int r[] = {0, 1, 2, 3, 0, 1, 2, 3};*/
const int /*led = 2, buttonpin = 3, */gpspin = 15, ledGPS = 17;


//add gps and sd
byte power;
byte channel;
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
  //0xB5, 0x62, 0x06, 0x08, 0x06, 0x00, 0x64, 0x00, 0x01, 0x00, 0x01, 0x00, 0x7A, 0x12, 0xB5, 0x62, 0x06, 0x08, 0x00, 0x00, 0x0E, 0x30;//(10Hz)

  // Baud Rate
  0xB5, 0x62, 0x06, 0x00, 0x14, 0x00, 0x01, 0x00, 0x00, 0x00, 0xD0, 0x08, 0x00, 0x00, 0x00, 0xE1, 0x00, 0x00, 0x07, 0x00, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0xE2, 0xE1, 0xB5, 0x62, 0x06, 0x00, 0x01, 0x00, 0x01, 0x08, 0x22 //Baud rate 57600 00 E1 00
  //..................................................................................0x80, 0x25, 0x00, ....set baud rate(xxxxxx(HEX))  ???..............
  //byte setPortRate[] = {0xB5, 0x62, 0x06, 0x00, 0x14, 0x00, 0x01, 0x00, 0x00, 0x00, 0xD0, 0x08, 0x00, 0x00, settingsArrayPointer[3], settingsArrayPointer[4], settingsArrayPointer[5], 0x00, 0x07, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
  //const char baud57600 [] PROGMEM = "PUBX,41,1,3,3,57600,0";

  //PortRate:
  //4800   = C0 12 00
  //9600   = 80 25 00
  //19200  = 00 4B 00  **SOFTWARESERIAL LIMIT FOR ARDUINO UNO R3!**
  //38400  = 00 96 00  **SOFTWARESERIAL LIMIT FOR ARDUINO MEGA 2560!**
  //57600  = 00 E1 00
  //115200 = 00 C2 01
  //230400 = 00 84 03
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

void setup()
{

  Serial.begin(57600);
  pinMode(gpspin, INPUT);
  pinMode(ledGPS, OUTPUT);
  pinMode(5, OUTPUT);
  pinMode(hb, OUTPUT);
  digitalWrite(5, LOW);
  digitalWrite(19, LOW);

  headmillis = millis();

  //GPS and sd
  serial.begin(9600);
  setupgps();
  serial.begin(57600);
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
    //while (1) {
    Serial.println(F("error opening file"));
    delay(200);
    //}
  }
  Serial.println(F("initialization done."));


  msgType = processGPS(); //read gps in every loop (put here due to the reading singel delay will  effect relay time
  gpstime = ubxMessage.navPosllh.iTOW;

  currentmillis = millis() - headmillis;
  transf2utc();
  writegpsToCard();

  hallo = analogRead(H1);
}


void loop() {
  digitalWrite(hb, LOW);

  if (digitalRead(gpspin) == HIGH) {
    digitalWrite(ledGPS, HIGH);
  }
  else {
    digitalWrite(ledGPS, LOW);
  }

  currentmillis = (millis() - headmillis) / 1000;
  //Serial.print(currentmillis); Serial.print(" ");

  hall = abs(analogRead(H1) - hallo);
  Serial.println(hall);

  if (hall > 5) {
    digitalWrite(hb, HIGH);
  }

  msgType = processGPS(); //read gps in every loop (put here due to the reading singel delay will  effect relay time
  gpstime = ubxMessage.navPosllh.iTOW;
  
  transf2utc();
  writegpsToCard();

  if (i % 10000 == 0) {
    if (logFile.size() > maxsize) {
      logFileName = nextLogFile();
    }
    logFile.close(); // close the file
  }
  
  f = currentmillis / i;
  i++;
}




void writegpsToCard() {
  currentmillis = millis() - headmillis;
  if (!logFile) {
    logFile = SD.open(logFileName, FILE_WRITE);
  }
  if (logFile) {
    logFile.print(currentmillis);
    logFile.print(",");
    logFile.print(hall);
    logFile.print(",");
    logFile.print(gpstime);
    logFile.print(",");
    logFile.print((TWtime[1]), 0);
    logFile.print(",");
    logFile.print((TWtime[2]), 0);
    logFile.print(",");
    logFile.print((TWtime[3]), 5);
    logFile.print("\n");
    logFile.print(f, 2);
    logFile.print("\n");
    //alogFile.close();
  }
}


String nextLogFile(void) {
  String filename;
  int logn = 0;
  for (int i = 0; i < 999; i++) {
    // Construct a file with PREFIX[Index].SUFFIX
    filename = String("power") + String(logn) + String(".csv");
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

void transf2utc() {

  double temp = 0;

  utctime = gpstime / 1000 + (utc * 3600) - 19; //utc&leap fix

  TWtime[0] = (int)(utctime / 24 / 3600); //week
  temp = utctime - (TWtime[0] * 24 * 3600);

  TWtime[1] = (int)(temp / 3600); //hr
  temp = utctime - (TWtime[0] * 24 * 3600) - (TWtime[1] * 3600);

  TWtime[2] = (int)(temp / 60); //min
  temp = utctime - (TWtime[0] * 24 * 3600) - (TWtime[1] * 3600) - (TWtime[2] * 60);

  TWtime[3] = temp; //sec

  logdata = gpstime;
}



void setupgps() { //setup UBLOX_INIT codes
  for (int i = 0; i < sizeof(UBLOX_INIT); i++) {
    serial.write( pgm_read_byte(UBLOX_INIT + i) );
    delay(5); // simulating a 38400baud pace (or less), otherwise commands are not accepted by the device.
  }
}


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
