void setup() {
  Serial.begin(115200);
  while (!Serial); // 等待序列埠連接

  Serial.println("=== Teensy 4.0: 64-bit Double to String/Char Test ===");

  // 測試 double 變數
  double testDouble = 123.4567890123456789;

  // 顯示原始 double 值
  Serial.print("Original double value: ");
  Serial.println(testDouble, 15);

  // 使用 sprintf 將 double 轉換為 char[]
  char doubleChar[64];
  sprintf(doubleChar, "%.15f", testDouble);
  Serial.print("As char array (sprintf): ");
  Serial.println(doubleChar);

  // 驗證轉換回 double
  double restoredDouble = atof(doubleChar);
  Serial.print("Restored double: ");
  Serial.println(restoredDouble, 15);

  // 比較原始與還原的 double 是否一致
  if (testDouble == restoredDouble) {
    Serial.println("✅ Double value preserved accurately!");
  } else {
    Serial.println("❌ Double value mismatch!");
  }
}

void loop() {
}
