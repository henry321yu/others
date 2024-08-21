/**
 * Serial Duplex 
 * by Tom Igoe. 
 * 
 * Sends a byte out the serial port when you type a key
 * listens for bytes received, and displays their value. 
 * This is just a quick application for testing serial data
 * in both directions. 
 */


import processing.serial.*;

Serial mySerial;      // The serial port
String read;    // Incoming serial data
float val1;
float val2;
float val3;
float val4;

void setup() {
  size(400, 300);
  // create a font with the third font available to the system:
  PFont myFont = createFont(PFont.list()[2], 14);
  textFont(myFont);

  // List all the available serial ports:
  printArray(Serial.list());

  // I know that the first port in the serial list on my mac
  // is always my  FTDI adaptor, so I open Serial.list()[0].
  // In Windows, this usually opens COM1.
  // Open whatever port is the one you're using.
  String portName = Serial.list()[0];
  mySerial = new Serial(this, portName, 115200);
}

void draw() {
  while (mySerial.available()>0) {
    read = mySerial.readString();

    if (read!=null) {
      background(0);

      read = read.trim();
      String[] values = split(read, '\t');
      val1 = float(values[0]);
      val2 = float(values[1]); 
      val3 = float(values[2]); 
      val4 = float(values[3]);   
      text("time: " + val1, 10, 20); 
      text("ang: " + val2, 10, 40);
      text("dis: " + val3, 10, 60);
      text("f: " + val4, 10, 80);
    }
  }
}
