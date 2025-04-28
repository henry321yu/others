#include <Wire.h>
#include <Adafruit_MLX90393.h>

// 建立 MLX90393 物件
Adafruit_MLX90393 mlx = Adafruit_MLX90393();

void setup() {
  Serial.begin(115200);
  while (!Serial) {
    delay(10); // 等待序列埠連接
  }

  Serial.println("MLX90393 磁場感測器初始化中...");
  if (!mlx.begin_I2C()) {
    Serial.println("無法找到 MLX90393 感測器，請檢查連接!");
    while (1) {
      delay(10);
    }
  }
  Serial.println("MLX90393 初始化成功!");
}

void loop() {
  sensors_event_t event;
  mlx.getEvent(&event);
  Serial.print("X: "); Serial.print(event.magnetic.x); Serial.print(" uT, ");
  Serial.print("Y: "); Serial.print(event.magnetic.y); Serial.print(" uT, ");
  Serial.print("Z: "); Serial.print(event.magnetic.z); Serial.println(" uT");
  
  delayMicroseconds(100); // 每 500ms 讀取一次
}
