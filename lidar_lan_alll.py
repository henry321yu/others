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

def send_adxl355():
    while True:
        ax, ay, az, temp = read_adxl355()
        message = f"ADXL355,{ax:.6f},{ay:.6f},{az:.6f},{temp:.2f}"
        for ip, _ in REMOTE_PC_LIST:
            sock_adxl.sendto(message.encode(), (ip, ADXL_PORT))
        time.sleep(0.01)

def send_camera():
    cap = cv2.VideoCapture(0)
    if not cap.isOpened():
        print("❌ 攝影機無法開啟")
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
            sock_pixel.sendto(pixel_msg.encode(), (ip, PIXEL_PORT))

        if frame_count % 2 == 0:
            success, jpeg = cv2.imencode('.jpg', resized, [int(cv2.IMWRITE_JPEG_QUALITY), 60])
            if success and len(jpeg) < 60000:
                for ip, _ in REMOTE_PC_LIST:
                    sock_img.sendto(jpeg.tobytes(), (ip, IMAGE_PORT))
        frame_count += 1
        time.sleep(interval)

# ========= LiDAR 資料接收轉發 =========
def forward_lidar(local_port, remote_port):
    recv_sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    recv_sock.bind(('', local_port))
    while True:
        data, _ = recv_sock.recvfrom(12000)
        for ip, _ in REMOTE_PC_LIST:
            sock = sock_lidar1 if remote_port == LIDAR1_PORT else sock_lidar2
            sock.sendto(data, (ip, remote_port))

# ========= 主程式 =========
if __name__ == "__main__":
    setup_adxl355()

    threading.Thread(target=send_adxl355, daemon=True).start()
    threading.Thread(target=send_camera, daemon=True).start()
    threading.Thread(target=forward_lidar, args=(2368, 2368), daemon=True).start()
    threading.Thread(target=forward_lidar, args=(2369, 2369), daemon=True).start()

    print("📡 系統啟動中... 按 Ctrl+C 結束。")
    try:
        while True:
            time.sleep(1)
    except KeyboardInterrupt:
        print("🛑 已手動中斷")
