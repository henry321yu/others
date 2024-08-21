String logFileName, accdata, gyrdata, magdata, logdata, mag2data, tmdata, halldata, flydata; // Rotordata;
extern float tempmonGetTemp(void);

void setup() {
  Serial.begin(115200); // Initialize serial output via USB
  delay(100);
  Serial.println(F("Serial.begin"));
}
void loop() {
  tmdata =String(tempmonGetTemp(), 2);
  Serial.println(tmdata);
  delay(1000);
}
