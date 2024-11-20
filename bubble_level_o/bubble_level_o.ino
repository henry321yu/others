#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire);

Adafruit_MPU6050 mpu;

void setup() {
  // 初始化 Serial
  Serial.begin(115200);

  // 初始化 SSD1306
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // 預設 OLED I2C 地址為 0x3C
    Serial.println(F("SSD1306 allocation failed"));
    for (;;);
  }
  display.clearDisplay();

  // 初始化 MPU6050
  if (!mpu.begin()) {
    Serial.println("MPU6050 init failed!");
    while (1);
  }

  // 配置 MPU6050
  mpu.setAccelerometerRange(MPU6050_RANGE_2_G);
  mpu.setGyroRange(MPU6050_RANGE_250_DEG);
  mpu.setFilterBandwidth(MPU6050_BAND_5_HZ);

  // 顯示啟動畫面
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println(F("Initializing..."));
  display.display();
  delay(1000);
}

void loop() {
  // 讀取加速度數據
  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);

  // 計算傾斜角度
  float angleX = atan2(a.acceleration.y, a.acceleration.z) * 180 / PI;
  float angleY = atan2(a.acceleration.x, a.acceleration.z) * 180 / PI;

  // 將角度映射到螢幕像素座標
  int bubbleX = map(angleX, -45, 45, 0, SCREEN_WIDTH - 1);
  int bubbleY = map(angleY, -45, 45, 0, SCREEN_HEIGHT - 1);

  // 限制電子氣泡範圍
  bubbleX = constrain(bubbleX, 0, SCREEN_WIDTH - 1);
  bubbleY = constrain(bubbleY, 0, SCREEN_HEIGHT - 1);

  // 繪製螢幕內容
  display.clearDisplay();
  display.drawCircle(SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2, 20, SSD1306_WHITE); // 圓形邊框
  display.fillCircle(bubbleX, bubbleY, 3, SSD1306_WHITE); // 電子氣泡
  display.setCursor(0, 0);
  display.setTextSize(1);
  display.printf("X: %.2f", angleX);
  display.setCursor(0, 10);
  display.printf("Y: %.2f", angleY);
  display.display();

  delay(50); // 更新頻率
}
