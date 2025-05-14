import cv2
import time
import socket
import os
import platform
import threading
import smbus2
from datetime import datetime

# ========= ADXL355 設定 =========
TEMP2, XDATA3, YDATA3, ZDATA3 = 0x06, 0x08, 0x0B, 0x0E
RESET, POWER_CTL, RANGE, SELF_TEST = 0x2F, 0x2D, 0x2C, 0x2E
bus = smbus2.SMBus(1)
ADXL355_ADDR = 0x1D

def setup_adxl355():
    bus.write_byte_data(ADXL355_ADDR, RESET, 0x52)
    time.sleep(0.1)
    bus.write_byte_data(ADXL355_ADDR, POWER_CTL, 0x00)
    bus.write_byte_data(ADXL355_ADDR, RANGE, 0x01)
    bus.write_byte_data(ADXL355_ADDR, SELF_TEST, 0x00)
    time.sleep(0.1)

def read_adxl355():
    data = bus.read_i2c_block_data(ADXL355_ADDR, TEMP2, 11)
    ax = ((data[2] << 12) | (data[3] << 4) | (data[4] >> 4)) & 0xFFFFF
    ay = ((data[5] << 12) | (data[6] << 4) | (data[7] >> 4)) & 0xFFFFF
    az = ((data[8] << 12) | (data[9] << 4) | (data[10] >> 4)) & 0xFFFFF
    ax = ax - 0x100000 if ax > 0x80000 else ax
    ay = ay - 0x100000 if ay > 0x80000 else ay
    az = az - 0x100000 if az > 0x80000 else az
    temp_raw = (data[0] << 8) | data[1]
    temp = ((1852 - temp_raw) / 9.05) + 27.2
    return ax / 0x3E800, ay / 0x3E800, az / 0x3E800, temp

# ========= 網路與設備清單 =========
REMOTE_PC_LIST = [
    ('10.241.0.114', 0),
    ('10.241.215.99', 0),
    ('10.241.180.148', 0),
    ('10.241.199.211', 0),
    ('10.241.183.64', 0),
    ('10.241.66.133', 0),
]

ADXL_PORT = 2370
IMAGE_PORT = 2385
PIXEL_PORT = 2386
LIDAR1_PORT = 2368
LIDAR2_PORT = 2369

# ========= 資料傳送 =========
sock_adxl = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sock_img = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sock_pixel = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sock_lidar1 = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sock_lidar2 = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sock_lidar1.bind(("", LIDAR1_PORT))
sock_lidar2.bind(("", LIDAR2_PORT))

# 初始化每台主機的已傳輸資料量（位元組）
sent_bytes_per_pc = {ip: 0 for ip, _ in REMOTE_PC_LIST}

# ========= 檢查主機是否 online =========
def ping(ip):
    param = "-n" if platform.system().lower() == "windows" else "-c"
    result = os.system(f"ping {param} 1 {ip} > /dev/null 2>&1")
    return result == 0

# 背景執行的 ping 檢查執行緒
def ping_checker():
    while True:
        for ip, _ in REMOTE_PC_LIST:
            online_status[ip] = ping(ip)
        time.sleep(5)

# ========= 傳送 ADXL355 資料 =========
def send_adxl355():
    global sent_bytes_per_pc
    while True:
        ax, ay, az, temp = read_adxl355()
        message = f"ADXL355,{ax:.6f},{ay:.6f},{az:.6f},{temp:.2f}"
        for ip, _ in REMOTE_PC_LIST:
            if online_status.get(ip, False):
                sent_bytes = sock_adxl.sendto(message.encode(), (ip, ADXL_PORT))
                sent_bytes_per_pc[ip] += sent_bytes
        time.sleep(0.01)

# ========= 傳送影像資料 =========
def send_camera():
    global sent_bytes_per_pc
    cap = cv2.VideoCapture(0)
    if not cap.isOpened():
        print("攝影機無法開啟")
        return

    frame_count = 0
    interval = 0.5
    while True:
        ret, frame = cap.read()
        if not ret:
            continue
        ts = datetime.now().strftime('%Y-%m-%d %H:%M:%S.%f')[:-3]
        cv2.putText(frame, ts, (30, 50), cv2.FONT_HERSHEY_SIMPLEX, 1, (0, 255, 0), 2)
        resized = cv2.resize(frame, (0, 0), fx=0.5, fy=0.5)
        gray = cv2.cvtColor(resized, cv2.COLOR_BGR2GRAY)
        pixel_val = int(gray[13, 50]) if gray.shape[0] > 13 and gray.shape[1] > 50 else 0
        pixel_msg = f"{ts},{pixel_val}"
        for ip, _ in REMOTE_PC_LIST:
            if online_status.get(ip, False):
                sent_bytes = sock_pixel.sendto(pixel_msg.encode(), (ip, PIXEL_PORT))
                sent_bytes_per_pc[ip] += sent_bytes

        if frame_count % 2 == 0:
            success, jpeg = cv2.imencode('.jpg', resized, [int(cv2.IMWRITE_JPEG_QUALITY), 60])
            if success and len(jpeg) < 60000:
                for ip, _ in REMOTE_PC_LIST:
                    if online_status.get(ip, False):
                        sent_bytes = sock_img.sendto(jpeg.tobytes(), (ip, IMAGE_PORT))
                        sent_bytes_per_pc[ip] += sent_bytes
        frame_count += 1
        time.sleep(interval)

# ========= LiDAR 資料接收轉發 =========
def lidar1():
    global sent_bytes_per_pc
    while True:
        data, _ = sock_lidar1.recvfrom(1500)
        for ip, _ in REMOTE_PC_LIST:
            if online_status.get(ip, False):
                sent_bytes = sock_lidar1.sendto(data, (ip, LIDAR1_PORT))
                sent_bytes_per_pc[ip] += sent_bytes

def lidar2():
    global sent_bytes_per_pc
    while True:
        data, _ = sock_lidar2.recvfrom(1500)
        for ip, _ in REMOTE_PC_LIST:
            if online_status.get(ip, False):
                sent_bytes = sock_lidar2.sendto(data, (ip, LIDAR2_PORT))
                sent_bytes_per_pc[ip] += sent_bytes

# ========= 顯示系統狀態 =========
def status_printer():
    while True:
        time.sleep(1)
        os.system("clear")
        print("=== Remote PC Online Status & Data Usage ===")
        for ip in REMOTE_PC_LIST:
            status = "🟢 Online" if online_status.get(ip, False) else "🔴 Offline"
            mb = sent_bytes_per_pc[ip] / (1024 * 1024)
            print(f"{ip}: {status} | Sent: {mb:.2f} MB")

# ========= 主程式 =========
if __name__ == "__main__":
    online_status = {}
    setup_adxl355()

    threading.Thread(target=send_adxl355, daemon=True).start()
    threading.Thread(target=send_camera, daemon=True).start()
    threading.Thread(target=lidar1, daemon=True).start()
    threading.Thread(target=lidar2, daemon=True).start()
    threading.Thread(target=ping_checker, daemon=True).start()
    threading.Thread(target=status_printer, daemon=True).start()

    print("系統啟動中... 按 Ctrl+C 結束。")
    try:
        while True:
            time.sleep(1)
    except KeyboardInterrupt:        
        sock_adxl.close()
        sock_img.close()
        sock_pixel.close()
        sock_lidar1.close()
        sock_lidar2.close()
        print("已手動中斷")
