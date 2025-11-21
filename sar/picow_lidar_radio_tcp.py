from machine import Pin, I2C, UART
import network
import socket
import time

# ==========================
# TFmini Plus I2C 設定
# ==========================
TFMINI_I2C_ADDR = 0x10
DATA_REG = 0x01


TF = UART(1, baudrate=115200, tx=Pin(8), rx=Pin(9))  # GP0=TX, GP1=RX

def read_tfmini():
    if TF.any():  # 檢查是否有資料
        data = TF.read(9)  # TFmini Plus UART 傳回 9 bytes
        if data and len(data) == 9:
            checksum = sum(data[0:8]) & 0xFF
            if checksum != data[8]:
                # 校驗失敗
                return None, None, None
            # 驗證開頭和校驗
            if data[0] == 0x59 and data[1] == 0x59:
                distance = data[2] + (data[3] << 8)  # 兩個 bytes 代表距離 (cm)
                strength = data[4] + (data[5] << 8)  # 信號強度
                temperature = data[6] + (data[7] << 8)  # 溫度
                return distance, strength, temperature
    return None, None

# ==========================
# HC-12 UART 設定與初始化
# ==========================

# 設定UART
hc12 = UART(0, tx=Pin(16), rx=Pin(17), baudrate=9600, parity=None, bits=8, stop=1, timeout=600)

# 設置 HC-12 模塊的 UART
hc12 = UART(0, 9600)  # 默認速率是 9600 bps
set_pin = Pin(18, Pin.OUT)  # 假設SET引腳連接到GP15
set_pin.value(0)  # 拉低SET引腳進入AT模式
time.sleep(0.1)  # 等待模塊穩定
hc12.write("AT+B115200\r\n")  # 將波特率設置為 115200 bps
time.sleep(0.1)  # 等待模塊回應
hc12 = UART(0, 115200)  # 115200 bps
time.sleep(0.1)  # 等待模塊回應
hc12.write("AT+B115200\r\n")  # 將波特率設置為 115200 bps
time.sleep(0.1)  # 等待模塊回應
hc12.write("AT+C057\r\n")  # 將頻道設置為 87
time.sleep(0.1)  # 等待模塊回應
hc12.write("AT+P8\r\n")
time.sleep(0.1)  # 等待模塊回應
# 檢查回應
if hc12.any():
    message = hc12.read()
    print(message.decode('utf-8'))

set_pin.value(1)
# ==========================
# Wi-Fi 設定
# ==========================
WIFI_SSID = "SGRC-WF"
WIFI_PWD  = "062383399"
TARGET_IP = "192.168.0.118"
TARGET_PORT = 5300
led = Pin("LED", Pin.OUT)

def wifi_connect():
    wlan = network.WLAN(network.STA_IF)
    wlan.active(True)
    wlan.connect(WIFI_SSID, WIFI_PWD)
    print(f"Connecting to Wi-Fi '{WIFI_SSID}'...")
    while not wlan.isconnected():
        time.sleep(0.1)
    ip = wlan.ifconfig()[0]
    print(f"Wi-Fi connected: IP={ip}")
    return wlan

wlan = wifi_connect()
sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

# ==========================
# 主循環 100Hz
# ==========================
print("Start reading TFmini+ and sending via HC-12 & Wi-Fi UDP...")

interval = 0.01  # 100Hz
while True:
    dist, strg, tem = read_tfmini()
    
    if dist is not None:
        dist_m = dist / 100   # 轉公尺
        tem_c = tem / 8 - 256 - 40
        msg = f"{dist_m},{strg},{tem_c}\n"
        
        # HC-12 傳送
        hc12.write(msg)
        
        # Wi-Fi UDP 傳送
        sock.sendto(msg.encode(), (TARGET_IP, TARGET_PORT))
        
        # LED 閃爍
        led.value(1)
        time.sleep(0.001)
        led.value(0)
        
        print(f"dis: {dist_m} m, strg: {strg}, tem: {tem_c} °C")
    else:
        print("TFmini read error")
    
    time.sleep(interval)
