//four channel relay test

int t = 1500; //set delay time
int relay[]={4,5,6,7}; // set relay pin
int ssr=3; // set ssr pin

void setup(){
  Serial.begin(9600);
  for (int i = 0; i <= 3 ; i++){
    pinMode(relay[i],OUTPUT); //initialize output pin
    digitalWrite(relay[i],HIGH); //turn the relay OFF,cause its low trigger
  }
}
void loop(){
  Serial.println("===Start===");
  for (int i = 0; i <= 3; i++)
  {
    digitalWrite(relay[i],LOW);
    Serial.print("relay ");Serial.println(relay[i]);
    Serial.println("on");
    delay(t);
    Serial.println("off");
    digitalWrite(relay[i],HIGH);    
    delay(500);
  }
  Serial.println("===Backward===");
  for (int i = 3; i >= 0; i--)
  {
    Serial.print("relay ");Serial.println(relay[i]);
    Serial.println("on");
    digitalWrite(relay[i],LOW);    
    delay(t);
    Serial.println("off");
    digitalWrite(relay[i],HIGH);    
    delay(500);
  }
}
