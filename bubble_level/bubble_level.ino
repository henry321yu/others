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
  float angleX = atan2(a.acceleration.x, a.acceleration.z) * 180 / PI;      ///my changes for different axis
  float angleY = atan2(a.acceleration.y, a.acceleration.z) * 180 / PI * -1;    ///my changes for different axis

  // 將角度映射到螢幕像素座標
  int bubbleX = map(angleX, -45, 45, 0, SCREEN_WIDTH);
  int bubbleY = map(angleY, -45, 45, 0, SCREEN_HEIGHT);

  // 限制電子氣泡範圍
  bubbleX = constrain(bubbleX, 0, SCREEN_WIDTH) + 35;
  bubbleY = constrain(bubbleY, 0, SCREEN_HEIGHT);

  // 判斷是否水平 (x 和 y 角度皆小於 1)
  bool isLevel = (abs(angleX) < 0.8 && abs(angleY) < 0.8);

  // 繪製螢幕內容
  display.clearDisplay();

  // 畫出水平圓形邊框
  display.drawCircle((SCREEN_WIDTH / 2) + 35, SCREEN_HEIGHT / 2, 24, SSD1306_WHITE);
  display.drawCircle((SCREEN_WIDTH / 2) + 35, SCREEN_HEIGHT / 2, 9, SSD1306_WHITE);

  // 畫出電子氣泡
  if (isLevel) {
    // 當水平時，畫出反白的圓點
    display.fillCircle(bubbleX, bubbleY, 7, SSD1306_BLACK);
    display.drawCircle(bubbleX, bubbleY, 7, SSD1306_WHITE);
  } else {
    // 非水平時，使用一般白色圓點
    display.fillCircle(bubbleX, bubbleY, 7, SSD1306_WHITE);
  }

  // 顯示角度資訊
  display.setTextSize(1);

  display.setCursor(0, 40);
  display.printf("X: %.2f", angleX);
  display.setCursor(0, 50);
  display.printf("Y: %.2f", angleY);

  display.setCursor(0, 0);
  display.printf("ax: %.2f", a.acceleration.x);
  display.setCursor(0, 10);
  display.printf("ay: %.2f", a.acceleration.y);
  display.setCursor(0, 20);
  display.printf("az: %.2f", a.acceleration.z);

  display.display();

  delay(50); // 更新頻率
}
