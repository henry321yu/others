import processing.serial.*;

int xPos = 1;
int lastxPos=1;
int lastheight=0;
float x;
float y;
long yy=0;
long i=0;
long ii=0;
float yfix;

void setup() {
  size(1200, 200);
  // create a font with the third font available to the system:
  PFont myFont = createFont(PFont.list()[2], 14);
  textFont(myFont);
  background(0);
}

void draw() {
  i++;
  y=ii;
  if (i%50==0) {
    if (i>300) {
      ii=ii-51;
    } else {
      ii=ii+51;
    }
  }

  mydraw();

  print(y);
  print('\t');
  print(yy);
  print('\t');
  print(yfix);
  print('\t');
  println(i);
}

void mydraw() {
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
}
