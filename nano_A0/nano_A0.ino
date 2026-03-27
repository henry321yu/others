void setup() {
  Serial.begin(115200);

  // 使用外部參考電壓（AREF = 約 2V）
  analogReference(EXTERNAL);
}

void loop() {
  int adcValue = readADC();
//  int adcValue = analogRead(A0);

  // AREF ≈ 2.0V（你的分壓）
  float voltage = adcValue * (2.0 / 1023.0);

  // 0~2V 對應 0~100% RH
  float humidity = (voltage / 2.0) * 100.0;

//  Serial.print("ADC: ");
//  Serial.print(adcValue);
//
//  Serial.print("  Voltage: ");
//  Serial.print(voltage, 3);
//
//  Serial.print(" V  Humidity: ");
//  Serial.print(humidity, 1);
//  Serial.println(" %");

  Serial.println(humidity, 2);
  
//  delay(1);
}
int readADC() {
  long sum = 0;
  for (int i = 0; i < 50; i++) {
    sum += analogRead(A0);
    delay(1);
  }
  return sum / 50;
}
