#include <Arduino.h>

void setup() {
  Serial.begin(9600);
  delay(1000);
  Serial.println("Rebooting now...");

  // 底層強制系統重啟
  *((volatile uint32_t *)0xE000ED0C) = (0x5FA << 16) | (1 << 2);  // 相當於 NVIC_SystemReset()
  while (1);  // 等待重啟
}

void loop() {}
