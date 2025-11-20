import network
import socket
import time
from machine import Pin

# -----------------------------
# Wi-Fi 設定
# -----------------------------
WIFI_SSID = "SGRC-WF"
WIFI_PWD  = "062383399"

# -----------------------------
# 目標主機設定
# -----------------------------
TARGET_IP = "192.168.0.107"
TARGET_PORT = 5300

# -----------------------------
# LED 設定
# -----------------------------
led = Pin("LED", Pin.OUT)

# -----------------------------
# 連線 Wi-Fi 函式
# -----------------------------
def wifi_connect():
    wlan = network.WLAN(network.STA_IF)
    wlan.active(True)
    wlan.connect(WIFI_SSID, WIFI_PWD)
    print(f"\nConnecting to Wi-Fi '{WIFI_SSID}'...")

    while not wlan.isconnected():
        time.sleep(0.1)

    ip = wlan.ifconfig()[0]
    print(f"Wi-Fi connected: SSID='{WIFI_SSID}', IP={ip}")
    return wlan

# -----------------------------
# 主程式
# -----------------------------
wifi_connect()

sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

start_time = time.ticks_ms()
interval = 0.01   # 100Hz

print("Start sending packets...")

while True:
    now = time.ticks_ms()
    elapsed = time.ticks_diff(now, start_time)
    msg = str(elapsed)
    print(msg)

    # 發送 UDP 封包
    sock.sendto(msg.encode(), (TARGET_IP, TARGET_PORT))

    # 讓 LED 閃一下
    led.value(1)
    time.sleep(0.001)   # 1ms 閃爍，不影響 100Hz
    led.value(0)

    time.sleep(interval)

