from machine import Pin, I2C, UART
import time

# ==========================
# TFmini Plus I2C 設定
# ==========================
TFMINI_I2C_ADDR = 0x10
DATA_REG = 0x01

i2c = I2C(0, scl=Pin(5), sda=Pin(4), freq=400000)  # 400kHz I2C

def read_tfmini_distance():
    """讀取 TFmini Plus 的距離資料"""
    try:
        buf = i2c.readfrom_mem(TFMINI_I2C_ADDR, DATA_REG, 9)
        distance = (buf[2] << 8) | buf[3]
        strength = (buf[4] << 8) | buf[5]
        mode = buf[6]
        return distance, strength, mode
    except Exception as e:
        return None, None, None

# ==========================
# HC-12 UART 設定與初始化
# ==========================
# 初始化 UART（先用 9600 進入 AT 模式）
uart = UART(0, baudrate=9600, tx=Pin(0), rx=Pin(1))
set_pin = Pin(15, Pin.OUT)  # 假設 SET 引腳連接到 GP15
set_pin.value(0)  # 拉低 SET 進入 AT 模式
time.sleep(0.2)   # 等待模塊穩定

# 設置 HC-12 波特率、頻道、功率
uart.write(b"AT+B115200\r\n")  # 設置 115200 bps
time.sleep(0.1)
uart.write(b"AT+C087\r\n")     # 設置頻道 87
time.sleep(0.1)
uart.write(b"AT+P8\r\n")       # 設置最高發射功率
time.sleep(0.1)

# 讀取 AT 回應（可選）
if uart.any():
    print(uart.read().decode('utf-8'))

# 設置完成後退出 AT 模式
set_pin.value(1)
time.sleep(0.1)

# 重新初始化 UART 為 115200 bps 傳輸模式
uart = UART(0, baudrate=115200, tx=Pin(0), rx=Pin(1))

# ==========================
# 主循環：100Hz 讀取距離並透過 HC-12 傳送
# ==========================
print("Start reading TFmini+ at 100Hz and sending via HC-12...")

while True:
    dist, strength, mode = read_tfmini_distance()
    
    if dist is not None:
        msg = f"{dist}\n"      # 只傳距離 (mm)
        uart.write(msg)
        print("Distance:", dist, "mm")
    else:
        print("TFmini read error")
    
    time.sleep(0.01)  # 100Hz
