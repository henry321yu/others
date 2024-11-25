from machine import Pin, UART
import time

# 定義 UART 和 SET PIN
hc12_tx_pin = 16  # HC-12 的 TX 接到 Pico 的 GPIO 21
hc12_rx_pin = 17  # HC-12 的 RX 接到 Pico 的 GPIO 20
set_pin = Pin(18, Pin.OUT)  # 設定模式的腳位

# 初始化 UART
hc12 = UART(0, baudrate=115200, tx=Pin(hc12_tx_pin), rx=Pin(hc12_rx_pin))

# 初始化 SET PIN
set_pin.low()  # 將 SET PIN 拉低進入設定模式
time.sleep(0.1)

# 設置 HC-12 的參數
print("HC-12 Resetting...")
hc12.write(b"AT+B115200\r\n")  # 設定波特率為 115200
time.sleep(0.1)
hc12.init(baudrate=115200)  # 更新 UART 波特率為 115200
time.sleep(0.1)

hc12.write(b"AT+C117\r\n")  # 設定通道 127
time.sleep(0.1)
hc12.write(b"AT+P8\r\n")    # 設定最大功率
time.sleep(0.1)

set_pin.high()  # 拉高 SET PIN 離開設定模式
print("HC-12 Configuration Done")
time.sleep(0.1)

# 主程式
print("Initialization complete")
hc12.write(b"done initialize\r\n")

while True:
    # HC-12 傳來的資料傳到 USB UART
    if hc12.any():
        data = hc12.read()
        if data:
            print(data.decode(), end="")
    time.sleep(0.001)
