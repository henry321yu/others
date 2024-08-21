/*
Plug & Play Cycle
This simple sketch turns each of the 8 relays on and then off, one at a time, and repeats.
Plug 8 lamps into the 8 outlets and they will turn on and off in succession.
This uses arrays and for statements to make it easy to manage 8 outputs. 
Used millis() for timing to avoid using delay().
Created August 12, 2015
by Palace Games www.palace-games.com
*/

int pinArray[] = {2,3,4,5,6,7,8,9}; // define the pins that will connect to the relays
int relayStateArray[] = {1,0,0,0,0,0,0,0}; // track the state of a relay (on or off). Start with the first one on.
byte nextRelay = 0; // flips to 1 when it's time for the next relay to turn on
unsigned long currentMillis = 0; // variable to track the current "time" -- milliseconds since last time the Arduino was reset
unsigned long previousMillis = 0; // variable to track the last time a relay state was changed
long onTime = 250; // milliseconds of on-time. Change this value to alter how long each relay stays on (the speed of the cycle).
int i = 0; // used for the "for" statements (loops that cycle through all 8 relays).

void setup() {
for(i=0;i<8;i++){  // set the pinMode to OUTPUT for the 8 pins that connect to the relays
pinMode(pinArray[i],OUTPUT);
}
}

void loop() {

currentMillis = millis(); // set this variable to the current Arduino time

for(i=0;i<8;i++){ 
  if((relayStateArray[i] == HIGH) && (currentMillis - previousMillis >= onTime)){ // If a relay is on, but has exceeded the time period...
  relayStateArray[i] = LOW; // turn it off...
  previousMillis = currentMillis; // record the time of this change in state to the relay...
  nextRelay = 1; // and set this variable "1" to indicate that it's time to turn the next relay on.
  }
 else if (nextRelay == 1){ // If it's time to turn on the next relay...
   relayStateArray[i] = HIGH; // turn it on...
   nextRelay = 0;   // and set variable to 0, until enough time has elapsed, per above.
   }
 digitalWrite(pinArray[i],relayStateArray[i]); // the above just changes the value in the state array. This turns the relay on or off based on that value.
  }

}
