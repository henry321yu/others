void setup() {
  Serial.begin(115200);
  while (!Serial); // 等待序列埠連接

  Serial.println("=== Teensy 4.0: 64-bit Double to String/Char Test ===");

  // 測試 double 變數
  double testDouble = 123.4567890123456789;

  // 顯示原始 double 值
  Serial.print("Original double value: ");
  Serial.println(testDouble, 15);

  // 將 double 轉換為 String
  String doubleString = String(testDouble, 15);
  Serial.print("As String: ");
  Serial.println(doubleString);

  // 將 double 轉換為 char 陣列
  char doubleChar[32];
  dtostrf(testDouble, 1, 15, doubleChar); // double, 最小寬度, 小數點後15位, 儲存到 char[]
  Serial.print("As char array: ");
  Serial.println(doubleChar);

  // 驗證是否一致
  Serial.println("\nValidation:");
  Serial.print("Double → String → Double: ");
  Serial.println(atof(doubleString.c_str()), 15); // 使用 atof 轉換 String 回 double

  Serial.print("Double → Char → Double: ");
  Serial.println(atof(doubleChar), 15); // 使用 atof 轉換 char[] 回 double

  Serial.println("\nExpected:");
  Serial.println("- The values printed above should match the original double value.");
}

void loop() {
}
