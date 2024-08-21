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
float[] val=new float[20];

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
      for (int i=0; i<2; i++) {
        val[i] = float(values[i]); 
        text(val[i], 10, 20*(i+1));
      }
    }
  }
}
