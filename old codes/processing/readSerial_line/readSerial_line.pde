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
int ii;

long yy=0; //for mydraw
float yfix;

void setup() {
  size(1200, 200);
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
    read = mySerial.readString();

    if (read!=null) {
      //background(0);  //for texttttttttttttttt

      //output.print(read);
      read = read.trim();
      String[] values = split(read, '\t');

      for (int i=0; i<4; i++) {
        val[i] = float(values[i]);
        //text(val[i], 10, 20*(i+1));

        int k=2;////  setttt pppppposition of plot numberrrrrrrrrrrr
        //Drawing a line from Last inByte to the new one.
        //stroke(127, 34, 255);     //stroke color
        //strokeWeight(4);        //stroke wider
        //line(lastxPos, lastheight, xPos, height - val[k]);
        //text(val[k], 10, 20);


        stroke(255, 0, 0);     //stroke color
        strokeWeight(3);        //stroke wider
        float ang = val[1];
        float dis = val[2];
        //ang=0;        dis=400;
        x=dis*cos(ang*-1/180*3.14159);
        y=dis*sin(ang*-1/180*3.14159);
        y=dis;



        stroke(255, 0, 0);     //stroke color
        strokeWeight(3);        //stroke wider
        line(lastxPos, lastheight, xPos, y+yfix);
        lastxPos=xPos;
        lastheight= int(y+yfix);
        if (y -yy< 0) {
          println("lower");
          yy=yy-height;
          yfix=yfix+height;
          background(0);  //Clear the screen.
          lastheight= 200;
        }
        if (y-yy>height) {
          println("higher");
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
        /*
        if (i==0) {
         output.print(values[0]);
         output.print('\t');
         output.print(values[1]);
         output.print('\t');
         output.print(values[2]);
         output.print('\t');
         output.print(values[3]);
         output.print('\n');
         }*/
      }
    }
  }
}

void mydraw() {
  stroke(255, 0, 0);     //stroke color
  strokeWeight(3);        //stroke wider

  y=y-1;

  if (y < 0) {
    println("lower");
    y=y+height;
    background(0);  //Clear the screen.
    lastheight= 200;
  }

  if (y > height) {
    println("higher");
    y=y-height;
    background(0);  //Clear the screen.
    lastheight= 0;
  }

  line(lastxPos, lastheight, xPos, y);

  stroke(255, 255, 255);     //stroke color
  ellipse(width / 2, height / 2, 2, 2);

  lastxPos=xPos;
  lastheight= int(y);


  if (xPos >= width) {
    xPos = 0;
    lastxPos= 0;
    background(0);  //Clear the screen.
  } else {
    // increment the horizontal position:
    xPos++;
  }
}
