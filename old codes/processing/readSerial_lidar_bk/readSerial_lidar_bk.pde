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
PrintWriter output;

Serial mySerial;      // The serial port
String read;    // Incoming serial data
float[] val=new float[20];
int xPos = 1;
int lastxPos=1;
int lastheight=0;
float x;
float y;
long i;
float t0;
float f;

float time;
float ang;
float dis;
float freq;

long yy=0; //for mydraw
float yfix;

void setup() {
  size(800, 800);
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
  background(0);


  String Name = "Lidarlog"+"_"+str(year())+str(month())+str(day())+"_"+str(hour())+"."+str(minute())+".txt";
  // Create file to save the captured data
  //output = createWriter(Name);
}

void draw() {
  while (mySerial.available()>0) {
    //read = mySerial.readString();
    read=mySerial.readStringUntil(10);



    if (read!=null) {
      //output.print(read);

      if (i%1000==0) {
        background(0);
      }

      String[] values = split(read, '\t');

      time = float( values[0]);
      ang =  float(values[1]);
      dis =  float(values[2]);
      freq =  float(values[3]);

      x=dis*cos(ang/180*3.14159);
      y=dis*sin(ang/180*3.14159);
      x=x*0.4;
      y=y*0.4;

      stroke(127, 34, 255);     //stroke color
      strokeWeight(4);        //stroke wider
      point(x+width/2, y+height/2);

      t0=millis();
      f=i/t0*1000;

      print(time);
      print('\t');
      print(ang);
      print('\t');
      print(dis);
      print('\t');
      print(freq);
      print('\t');
      print(f);

      print('\t');
      print('\t');

      print(x);
      print('\t');
      print(y);

      print('\n');
      i++;
    }
  }
}

void mydraw() {
  stroke(255, 0, 0);     //stroke color
  strokeWeight(3);        //stroke wider
  line(lastxPos, lastheight, xPos, y+yfix);
  lastxPos=xPos;
  lastheight= int(y+yfix);
  if (y -yy< 0) {
    //println("lower");
    yy=yy-height;
    yfix=yfix+height;
    background(0);  //Clear the screen.
    lastheight= 200;
  }
  if (y-yy>height) {
    //println("higher");
    yy=yy+height;
    yfix=yfix-height;
    background(0);  //Clear the screen.
    lastheight= 0;
  }
  if (xPos >= width) {
    xPos = 0;
    lastxPos= 0;
    background(0);  //Clear the screen.
  } else {
    // increment the horizontal position:
    xPos++;
  }
}
