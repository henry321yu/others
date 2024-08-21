int rinA = 2; // One quadrature pin
int rinB = 3; // the other quadrature pin
int lastR, Read;
double Rotor = 0;

void setup() {

  // set DIO pins
  pinMode(rinA, INPUT);
  pinMode(rinB, INPUT);

  // Attach interrupt to pin A
  //attachInterrupt(0, UpdateRotation, FALLING);

  // Use serial port to keep user informed of rotation
  Serial.begin(38400);
  Read = digitalRead(rinB) << 1 | digitalRead(rinA);
  Read = graytobin(Read, 2);
  lastR = Read;
}
void loop() {
  Read = digitalRead(rinB) << 1 | digitalRead(rinA);
  covert();
  Serial.print ("Rotor : "); Serial.println (Rotor);
}

void covert() {
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
  if (abs(Rotor) == 360)
    Rotor = 0;
  lastR = Read;

}
/*void covert() { //new
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
}*/

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
