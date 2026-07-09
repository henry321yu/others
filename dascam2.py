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
    ('10.241.0.114', 0), # my office
    ('10.241.215.99', 0),  # dell
    # ('10.241.180.148', 0), # my pc
    ('10.241.199.211', 0), # fly office
    # ('10.241.183.64', 0), # ipc
    # ('10.241.66.133', 0), # broken
    # ('10.241.178.17', 0), # my phone
    ('10.241.135.1', 0), # my new office
]

ADXL_PORT = 2870
IMAGE_PORT = 2885
PIXEL_PORT = 2886

sock_adxl = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sock_img = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sock_pixel = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

# ========= 狀態變數 =========
online_status = {ip: True for ip, _ in REMOTE_PC_LIST}
avg20 = avg30 = avg40 = avg50 = 100
adxl_sent = img_sent = pixel_sent = 0

def ping(ip):
    param = "-n" if platform.system().lower() == "windows" else "-c"
    return os.system(f"ping {param} 1 {ip} > /dev/null 2>&1") == 0

def ping_checker():
    while True:
        for ip, _ in REMOTE_PC_LIST:
            online_status[ip] = ping(ip)
        time.sleep(5)

def send_adxl355():
    global adxl_sent
    last_az = None
    PREFIX = "ADXL355,"

    while True:
        ax, ay, az, temp = read_adxl355()

        if az == last_az:
            continue
        last_az = az

        message = PREFIX + "%.6f,%.6f,%.6f,%.2f" % (ax, ay, az, temp)

        for ip, _ in REMOTE_PC_LIST:
            if online_status[ip]:
                sock_adxl.sendto(message.encode(), (ip, ADXL_PORT))
                adxl_sent += 1
        time.sleep(0.0005)

def send_camera():
    global img_sent, pixel_sent, avg20, avg30, avg40, avg50
    cap = None
    frame_count = 0
    interval = 0.2
    while True:
        if cap is None or not cap.isOpened():
            cap = cv2.VideoCapture(0, cv2.CAP_V4L2)
            time.sleep(2)
            continue
        ret, frame = cap.read()
        if not ret:
            cap.release()
            cap = None
            continue
        ts = datetime.now().strftime('%Y-%m-%d %H:%M:%S.%f')[:-3]
        # cv2.putText(frame, ts, (10, 30), cv2.FONT_HERSHEY_SIMPLEX, 1.0, (255, 255, 255), 2)
        # resized = cv2.resize(frame, (0, 0), fx=0.5, fy=0.5)
        cv2.putText(frame, ts, (10, 20), cv2.FONT_HERSHEY_SIMPLEX, 0.5, (255, 255, 255), 1)
        resized = cv2.resize(frame, (0, 0), fx=1, fy=1)
        gray = cv2.cvtColor(resized, cv2.COLOR_BGR2GRAY)
        if gray.shape[0] > 50 and gray.shape[1] > 100:
            avg20 = int(gray[188, 110:305].mean()) #日 近側 roi
            avg30 = int(gray[136, 120:290].mean()) #日 遠側 roi_1
            avg40 = int(gray[146, 122:310].mean()) #夜 近側 roi_2
            avg50 = int(gray[101, 100:300].mean()) #夜 遠側 roi_3
        else:
            avg20 = avg30 = avg40 = avg50 = 0
        pixel_msg = f"{ts},{avg20},{avg30},{avg40},{avg50}"
        for ip, _ in REMOTE_PC_LIST:
            if online_status[ip]:
                sock_pixel.sendto(pixel_msg.encode(), (ip, PIXEL_PORT))
                pixel_sent += 1
        if frame_count % 2 == 0:
            success, jpeg = cv2.imencode('.jpg', resized, [int(cv2.IMWRITE_JPEG_QUALITY), 80])
            if success and len(jpeg) < 60000:
                for ip, _ in REMOTE_PC_LIST:
                    if online_status[ip]:
                        sock_img.sendto(jpeg.tobytes(), (ip, IMAGE_PORT))
                        img_sent += 1
        frame_count += 1
        time.sleep(interval)

def print_status():
    while True:
        os.system('cls' if platform.system().lower() == 'windows' else 'clear')
        print(f"  avg20 = {avg20}")
        print(f"  ADXL355: {adxl_sent} | 圖像: {img_sent} | 像素: {pixel_sent}")
        for ip, _ in REMOTE_PC_LIST:
            status = "Online" if online_status[ip] else "Offline"
            print(f"  {ip} : {status}")
        time.sleep(10)

if __name__ == "__main__":
    time.sleep(10)
    setup_adxl355()
    threading.Thread(target=ping_checker, daemon=True).start()
    threading.Thread(target=send_adxl355, daemon=True).start()
    threading.Thread(target=send_camera, daemon=True).start()
    threading.Thread(target=print_status, daemon=True).start()
    print("🟢 系統啟動中... Ctrl+C 可中斷")
    try:
        while True:
            time.sleep(1)
    except KeyboardInterrupt:
        sock_adxl.close()
        sock_img.close()
        sock_pixel.close()
        print("🔴 手動中斷")