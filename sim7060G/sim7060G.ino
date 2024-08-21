#include <SoftwareSerial.h>

// 定义软件串口引脚（如果使用软件串口）
SoftwareSerial sim7060(1, 0); // RX, TX (调整引脚以匹配你的连接)

// 初始化
void setup() {
  Serial.begin(9600);     // 用于与PC通信的串口
  sim7060.begin(9600);    // 用于与SIM7060G通信的串口

  Serial.println("Initializing SIM7060G...");

/*
  // 发送AT指令并检查响应
  sendATCommand("AT");
  sendATCommand("AT+CPIN?");
  sendATCommand("AT+CSQ");

  // 设置APN（替换为你自己的APN）
  sendATCommand("AT+CGDCONT=1,\"IP\",\"15923\"");

  // 激活PDP上下文
  sendATCommand("AT+CGACT=1,1");

  // 检查网络连接状态
  sendATCommand("AT+CGATT?");
  
  // 获取IP地址
  sendATCommand("AT+CIFSR");

  // 测试结束
  Serial.println("Setup complete.");
  */
}

// 循环函数
void loop() {
  // 这里可以添加更多测试代码
}

// 发送AT指令并打印响应
void sendATCommand(String command) {
  sim7060.println(command);
  delay(1000);  // 等待模块响应
  while (sim7060.available()) {
    String response = sim7060.readString();
    Serial.println(response);  // 输出模块的响应
  }
}
