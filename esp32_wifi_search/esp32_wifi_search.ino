#include <ESP8266WiFi.h> // 引入 ESP8266 WiFi 庫

void setup() {
  // 初始化串口監視器
  Serial.begin(115200);
  delay(100);

  // 設定 WiFi 模式為 STA (Station)
  WiFi.mode(WIFI_STA);
  WiFi.disconnect(); // 斷開現有的連接以便進行掃描

  Serial.println("Setup done");
}

void loop() {
  Serial.println("Scanning for WiFi networks...");
  int n = WiFi.scanNetworks(); // 掃描周圍的 WiFi 熱點

  if (n == 0) {
    Serial.println("No networks found");
  } else {
    Serial.print(n);
    Serial.println(" networks found");
    for (int i = 0; i < n; i++) {
      Serial.print("SSID: ");
      Serial.print(WiFi.SSID(i));
      Serial.print(" (RSSI: ");
      Serial.print(WiFi.RSSI(i)); // 信號強度
      Serial.print(") - ");

      // 判斷網絡的加密方式
      switch (WiFi.encryptionType(i)) {
        case ENC_TYPE_NONE:
          Serial.println("Open");
          break;
        case ENC_TYPE_WEP:
          Serial.println("WEP");
          break;
        case ENC_TYPE_TKIP:
          Serial.println("WPA/PSK");
          break;
        case ENC_TYPE_CCMP:
          Serial.println("WPA2/PSK");
          break;
        case ENC_TYPE_AUTO:
          Serial.println("Auto");
          break;
        default:
          Serial.println("Unknown");
          break;
      }
      delay(10);
    }
  }
  delay(1000); // 每 5 秒重新掃描一次
}
