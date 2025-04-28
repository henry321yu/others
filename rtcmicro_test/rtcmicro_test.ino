#include <TimeLib.h>

void setup() {
  Serial.begin(9600);
  setSyncProvider(getTeensy3Time); // 使用 Teensy 的 RTC 同步時間
}

void loop() {
  // 獲取 RTC 的時間
  time_t currentTime = now();
  
  // 獲取自整秒以來的毫秒數
  unsigned long ms_since_second = millis() % 1000;

  // 輸出 RTC 時間與毫秒數
  Serial.print("RTC Time: ");
  Serial.print(hour(currentTime));
  Serial.print(":");
  Serial.print(minute(currentTime));
  Serial.print(":");
  Serial.print(second(currentTime));
  Serial.print(".");
  Serial.println(ms_since_second);

  delay(10); // 每 10 毫秒更新
}
time_t getTeensy3Time() {
  return Teensy3Clock.get();
}
