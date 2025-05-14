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

adxl_sent = 0
img_sent = 0
pixel_sent = 0
lidar1_sent = 0
lidar2_sent = 0

def send_adxl355():
    global adxl_sent
    while True:
        ax, ay, az, temp = read_adxl355()
        message = f"ADXL355,{ax:.6f},{ay:.6f},{az:.6f},{temp:.2f}"
        for ip, _ in REMOTE_PC_LIST:
            sock_adxl.sendto(message.encode(), (ip, ADXL_PORT))
            adxl_sent += 1
        time.sleep(0.01)

def send_camera():
    global img_sent, pixel_sent
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
            sock_pixel.sendto(pixel_msg.encode(), (ip, PIXEL_PORT))
            pixel_sent += 1

        if frame_count % 2 == 0:
            success, jpeg = cv2.imencode('.jpg', resized, [int(cv2.IMWRITE_JPEG_QUALITY), 60])
            if success and len(jpeg) < 60000:
                for ip, _ in REMOTE_PC_LIST:
                    sock_img.sendto(jpeg.tobytes(), (ip, IMAGE_PORT))
                    img_sent += 1
        frame_count += 1
        time.sleep(interval)

# ========= LiDAR 資料接收轉發 =========
def lidar1():
    global lidar1_sent
    while True:
        data, _ = sock_lidar1.recvfrom(1500)
        for ip, _ in REMOTE_PC_LIST:
            if is_reachable(ip, LIDAR1_PORT):
                sock_lidar1.sendto(data, (ip, LIDAR1_PORT))
                lidar1_sent += 1

def lidar2():
    global lidar2_sent
    while True:
        data, _ = sock_lidar2.recvfrom(1500)
        for ip, _ in REMOTE_PC_LIST:
            if is_reachable(ip, LIDAR2_PORT):
                sock_lidar2.sendto(data, (ip, LIDAR2_PORT))
                lidar2_sent += 1

# ========= 驗證主機可否收 =========
def is_reachable(ip, port, timeout=0.05):
    try:
        test_sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        test_sock.settimeout(timeout)
        test_sock.sendto(b'', (ip, port))
        return True
    except Exception:
        return False
    finally:
        test_sock.close()

# ========= 主程式 =========
def print_status():
    global adxl_sent, img_sent, pixel_sent, lidar1_sent, lidar2_sent
    while True:
        os.system('cls' if platform.system().lower() == 'windows' else 'clear')
        print(f"  系統狀態：")
        print(f"  ADXL355 傳送數量: {adxl_sent}")
        print(f"  影像傳送數量: {img_sent}")
        print(f"  像素值傳送數量: {pixel_sent}")
        print(f"  LiDAR1 傳送數量: {lidar1_sent}")
        print(f"  LiDAR2 傳送數量: {lidar2_sent}")
        print(f"  --------------------")
        time.sleep(10)

if __name__ == "__main__":
    setup_adxl355()

    threading.Thread(target=send_adxl355, daemon=True).start()
    threading.Thread(target=send_camera, daemon=True).start()
    threading.Thread(target=lidar1, daemon=True).start()
    threading.Thread(target=lidar2, daemon=True).start()
    threading.Thread(target=print_status, daemon=True).start()

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
