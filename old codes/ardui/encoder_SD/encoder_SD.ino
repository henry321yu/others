#include <SD.h>
#include <SPI.h>

const int SD_CS = 10; // Pin 10 on Arduino Uno
int A = 2; // One quadrature pin
int B = 3; // the other quadrature pin
int lastR, Read;
int fade;
double Rotor = 0, time, f;
String logFileName;
const int led = 5; //
unsigned long i = 0, t0;
File logFile;

void setup() {
  Serial.begin(38400);
  SPI.begin();

  if (!SD.begin(SD_CS)) {
    Serial.println("Card failed, or not present");
  }
  pinMode(SD_CS, OUTPUT);

  logFileName = nextLogFile();
  logFile = SD.open(logFileName, FILE_WRITE);
  if (logFile) {
    Serial.println(F("writing"));
    //test0.close(); // close the file
  }
  // if the file didn't open, print an error:
  else {
    Serial.println("error opening test.txt");
  }

  pinMode(A, INPUT);
  pinMode(B, INPUT);

  // Attach interrupt to pin A
  //attachInterrupt(0, UpdateRotation, FALLING);

  // Use serial port to keep user informed of rotation
  Read = digitalRead(B) << 1 | digitalRead(A);
  Read = graytobin(Read, 2);
  lastR = Read;


  pinMode(led, OUTPUT);
  pinMode(4, OUTPUT);
  digitalWrite(4, LOW);
  digitalWrite(led, HIGH);
  delay(2000);
  digitalWrite(led, LOW);

  delay(1000);
  t0=millis();
}
void loop() {
  Read = digitalRead(B) << 1 | digitalRead(A);
  covert();
  fade=map(Rotor,0,360,0,255);
  analogWrite(led, fade);
  Serial.print (F("Rotor : ")); Serial.println (Rotor);

  if (!logFile) {
    logFile = SD.open(logFileName, FILE_WRITE);
    //Serial.println(F("file opened"));
  }
  //delay(1);
  if (logFile) {
    //Serial.println(F("writing"));
    //digitalWrite(led, HIGH);
    logFile.println(Rotor);

    //logFile.close(); // close the file
  }
  // if the file didn't open, print an error:
  else {
    Serial.println(F("error opening runf.txt"));
    //return;
  }
  //digitalWrite(led, LOW);
  delayMicroseconds(4050);//200hz
  timer();
}

String nextLogFile(void)
{
  String filename;
  int logn = 0;

  for (int i = 0; i < 999; i++) {
    // Construct a file with PREFIX[Index].SUFFIX
    filename = String("runf");
    filename += String(logn);
    filename += ".";
    filename += String("txt");
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

/*void covert() {
  Read = graytobin(Read, 2);
  if ((Read > lastR) || (Read == 0 && lastR == 3)) {
    if (Read == 3 && lastR == 0) {
    }
    else
      Rotor += 11.25;
  }
  if ((Read < lastR) || (Read == 3 && lastR == 0)) {
    if (Read == 0 && lastR == 3) {
    }
    else
      Rotor -= 11.25;
  }
  if (abs(Rotor) >= 360)
    Rotor = 0;
  lastR = Read;

}*/
void covert() {
  Read = graytobin(Read, 2);
  if (lastR != Read) {
    if (lastR == 0) {
      if (Read == 3) {
        Rotor -= 0.75;
      }
      else {
        Rotor += 0.75 * abs(Read-lastR);
      }
    }
    if (lastR == 1) {
      if (Read == 0) {
        Rotor -= 0.75;
      }
      else {
        Rotor += 0.75 * abs(Read-lastR);
      }
    }
    if (lastR == 2) {
      if (Read == 1) {
        Rotor -= 0.75;
      }
      else {
        Rotor += 0.75 * abs(Read-lastR);
      }
    }
    if (lastR == 3) {
      if (Read == 2) {
        Rotor -= 0.75;
      }
      else if (Read == 0)
        Rotor += 0.75;
      else {
        Rotor += 0.75 * abs(Read-lastR);
      }
    }
  }
  if (abs(Rotor) == 360)
    Rotor = 0;
  lastR = Read;
}

int graytobin(int grayVal, int nbits ) {
  // Bn-1 = Bn XOR Gn-1   source of method:  http://stackoverflow.com/questions/5131476/gray-code-to-binary-conversion but include correction
  int binVal = 0;
  bitWrite(binVal, nbits - 1, bitRead(grayVal, nbits - 1)); // MSB stays the same
  for (int b = nbits - 1; b > 0; b-- ) {
    // XOR bits
    if (bitRead(binVal, b) == bitRead(grayVal, b - 1)) { // binary bit and gray bit-1 the same
      bitWrite(binVal, b - 1, 0);
    }
    else {
      bitWrite(binVal, b - 1, 1);
    }
  }
  return binVal;
}

void timer() {
  time = (millis() - t0) * 0.001;
  //Serial.print(time,3);
  //Serial.print("    i= ");
  //Serial.println(i);
  i++;
  if (i % 100 == 0) {
    logFile.close(); // close the file
    if (i % 1000 == 0) {
      f = i / time;
      Serial.print(F("freqency= ")); Serial.print(f); Serial.println(F(" Hz"));
      //i = 0;
    }
  }
}
