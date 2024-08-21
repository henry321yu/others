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
int ii=0;
float[] xa=new float[1000];
float[] ya=new float[1000];
int plotnum;

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
  printArray(Serial.list());

  String portName = Serial.list()[0];
  mySerial = new Serial(this, portName, 115200);
  background(0);


  String Name = "Lidarlog"+"_"+str(year())+str(month())+str(day())+"_"+str(hour())+"."+str(minute())+".txt";
  output = createWriter(Name);
  stroke(255, 100, 255);     //stroke color
  strokeWeight(3);        //stroke wider
}

void draw() {
  while (mySerial.available()>0) {
    read=mySerial.readStringUntil(10);

    if (read!=null) {
      String[] values = split(read, '\t');
      if (values.length>=4) {

        time = float(values[0]);
        ang =  float(values[1]);
        dis =  float(values[2]);
        freq =  float(values[3]);

        t0=millis();
        f=i/t0*1000;

        x=dis*cos(ang/180*3.14159);
        y=dis*sin(ang/180*3.14159);

        xa[ii]=x;
        ya[ii]=y;

        float plotsize=0.4;  // plot size control
        plotnum=200; // must <999

        if (ii>=plotnum) {
          background(0);
          for (int j=0; j<=plotnum; j++) {
            point(xa[j]*plotsize+ width/2, ya[j]*plotsize+height/2);
          }
          plottext();

          ii=0;
          i++;
        }
        savedata();


        print(time);
        print('\t');
        print(ang);
        print('\t');
        print(dis);
        print('\t');
        print(freq);
        print('\t');
        print(f);

        print('\n');
        ii++;
      }
    }
  }
}

void plottext() {
  int testx=10;
  int numx=60;
  text("time", testx, 20);
  text(time, numx, 20);
  text("ang", testx, 40);
  text(ang, numx, 40);
  text("dis", testx, 60);
  text(dis, numx, 60);
  text("freq", testx, 80);
  text(freq, numx, 80);
  
  text("t", testx, 120);
  text(t0/1000, numx, 120);
  text("x", testx, 140);
  text(x, numx, 140);
  text("y", testx, 160);
  text(y, numx, 160);
  text("f", testx, 180);
  text(f, numx, 180);
  text("dots ps", testx, 200);
  text(plotnum*f, numx, 200);
  //text("dots pf", testx, 220);
  //text(plotnum, numx+4, 220); ////wtf???
}

void savedata() {
  output.print(time);
  output.print('\t');
  output.print(ang);
  output.print('\t');
  output.print(dis);
  output.print('\t');
  output.print(freq);
  output.print('\n');
}
