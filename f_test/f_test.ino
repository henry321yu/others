float f = 0;    // 頻率 (Hz)
float alpha = 2.0 / (10 + 1);  // 指數移動平均的平滑係數 10為資料數
unsigned long lastTime = 0;   // 上一次讀取時間
long i;
int dd = 0;

void setup() {
  Serial.begin(115200);
  lastTime = micros();
}

void loop() {
  unsigned long now = micros();
  unsigned long period = now - lastTime;  // 計算當前週期 (微秒)
  lastTime = now;

  if (period > 0) {
    float currentf = 1e6 / period;  // 計算當前頻率 (Hz)
    f = (alpha * currentf) + ((1 - alpha) * f);  // 指數移動平均
  }
  i++;
  double t =millis();
  t=t/1e3;
  if (i % 1000 == 0)
    dd += 5;

  Serial.printf("%.3f %d %d %.5f",t, i,dd, f); // 輸出當前頻率
  Serial.println("");  // 輸出當前頻率
  delay(dd);  // 這裡的 delay 視應用需求調整
}
