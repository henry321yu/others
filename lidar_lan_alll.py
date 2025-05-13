import socket
import time
import os
import platform
import threading
import smbus2
import cv2
from datetime import datetime

# ========= ADXL355 I2C 設定 =========
TEMP2 = 0x06
XDATA3 = 0x08
YDATA3 = 0x0B
ZDATA3 = 0x0E
RESET = 0x2F
POWER_CTL = 0x2D
RANGE = 0x2C
SELF_TEST = 0x2E

bus = smbus2.SMBus(1)
Device_Address = 0x1D  # ADXL355 I2C 位址

def setup_355_m():
    bus.write_byte_data(Device_Address, RESET, 0x52)
    time.sleep(0.1)
    bus.write_byte_data(Device_Address, POWER_CTL, 0x00)
    time.sleep(0.03)
    bus.write_byte_data(Device_Address, RANGE, 0x01)
    time.sleep(0.03)
    bus.write_byte_data(Device_Address, SELF_TEST, 0x00)
    time.sleep(0.1)

def read_355_m():
    var = bus.read_i2c_block_data(Device_Address, TEMP2, 11)
    ax = (var[2] << 12 | var[3] << 4 | var[4] >> 4)
    ay = (var[5] << 12 | var[6] << 4 | var[7] >> 4)
    az = (var[8] << 12 | var[9] << 4 | var[10] >> 4)
    rangee = 0x3E800  # 2g 範圍

    for v in ('ax', 'ay', 'az'):
        if locals()[v] > 0x80000:
            locals()[v] -= 0x100000

    ax = ax / rangee
    ay = ay / rangee
    az = az / rangee

    temp_raw = (var[0] << 8 | var[1])
    temp = ((1852 - temp_raw) / 9.05) + 27.2
    return ax, ay, az, temp

# ========= 網路設定 =========
ACCPORT_ACCEL = 2370
ACCPORT_PIXEL = 2386
ACCPORT_IMAGE = 2385

REMOTE_PC_LIST = [
    ('10.241.0.114', None),
    ('10.241.215.99', None),
    ('10.241.180.148', None),
    ('10.241.199.211', None),
    ('10.241.183.64', None),
    ('10.241.66.133', None),
]

# ========= Global Socket 和狀態 =========
sock_accel = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sock_pixel = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sock_image = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

online_status = {ip: False for ip, _ in REMOTE_PC_LIST}
sent_bytes_per_pc = {ip: 0 for ip, _ in REMOTE_PC_LIST}
total_bytes_sent = 0
packet_count = 0

# ========= 工具函式 =========
def ping(ip):
    param = "-n" if platform.system().lower() == "windows" else "-c"
    return os.system(f"ping {param} 1 {ip} > /dev/null 2>&1") == 0

def ping_checker():
    while True:
        for ip, _ in REMOTE_PC_LIST:
            online_status[ip] = ping(ip)
        time.sleep(5)

def clear_screen():
    os.system('cls' if platform.system().lower() == 'windows' else 'clear')

def display_status():
    global total_bytes_sent, packet_count
    clear_screen()
    print("🛰️ 資料傳送中")
    print(f"📦 總封包數：{packet_count}")
    print(f"📊 總傳輸量：{total_bytes_sent / (1024 * 1024):.2f} MB")
    for ip in sent_bytes_per_pc:
        status = "online" if online_status[ip] else "offline"
        mb = sent_bytes_per_pc[ip] / (1024 * 1024)
        print(f"  {ip} ： {mb:.2f} MB  {status}")
    print("-" * 40)

# ========= ADXL355 傳送執行緒 =========
def accel_sender():
    global packet_count, total_bytes_sent
    setup_355_m()
    last_display_time = time.time()

    while True:
        ax, ay, az, temp = read_355_m()
        message = f"ADXL355,{ax:.6f},{ay:.6f},{az:.6f},{temp:.2f}"
        data = message.encode('utf-8')

        for ip, _ in REMOTE_PC_LIST:
            if not online_status[ip]:
                continue
            try:
                sent = sock_accel.sendto(data, (ip, ACCPORT_ACCEL))
                total_bytes_sent += sent
                sent_bytes_per_pc[ip] += sent
            except:
                pass

        packet_count += 1
        if time.time() - last_display_time >= 1:
            display_status()
            last_display_time = time.time()
        time.sleep(0.01)

# ========= 攝影機傳送執行緒 =========
def camera_sender():
    capture_interval = 0.5
    sent_interval = 1.0
    resize_scale = 0.5
    jpeg_quality = 60
    frame_send_interval = int(sent_interval / capture_interval)
    frame_count = 0

    cap = cv2.VideoCapture(0)
    if not cap.isOpened():
        print("❌ 無法開啟攝影機")
        return

    while True:
        ret, frame = cap.read()
        if not ret:
            print("⚠️ 讀取影像失敗")
            break

        now = datetime.now()
        timestamp = now.strftime('%Y-%m-%d %H:%M:%S.%f')[:-3]
        cv2.putText(frame, timestamp, (30, 50), cv2.FONT_HERSHEY_SIMPLEX, 1, (0, 255, 0), 2)

        new_w = int(frame.shape[1] * resize_scale)
        new_h = int(frame.shape[0] * resize_scale)
        resized = cv2.resize(frame, (new_w, new_h), interpolation=cv2.INTER_AREA)
        gray = cv2.cvtColor(resized, cv2.COLOR_BGR2GRAY)

        # 傳像素值
        if 13 < gray.shape[0] and 50 < gray.shape[1]:
            pixel_val = int(gray[13, 50])
            message = f"{timestamp},{pixel_val}"
            for ip, _ in REMOTE_PC_LIST:
                if online_status[ip]:
                    try:
                        sock_pixel.sendto(message.encode(), (ip, ACCPORT_PIXEL))
                    except:
                        pass

        # 傳影像
        if frame_count % frame_send_interval == 0:
            encode_param = [int(cv2.IMWRITE_JPEG_QUALITY), jpeg_quality]
            success, jpeg_bytes = cv2.imencode('.jpg', resized, encode_param)
            if success and len(jpeg_bytes) <= 60000:
                frame_bytes = jpeg_bytes.tobytes()
                for ip, _ in REMOTE_PC_LIST:
                    if online_status[ip]:
                        try:
                            sock_image.sendto(frame_bytes, (ip, ACCPORT_IMAGE))
                        except:
                            pass

        frame_count += 1
        time.sleep(capture_interval)

# ========= 主程式 =========
if __name__ == "__main__":
    try:
        # 啟動 ping 檢查
        threading.Thread(target=ping_checker, daemon=True).start()
        # 啟動 ADXL355 傳送執行緒
        threading.Thread(target=accel_sender, daemon=True).start()
        # 啟動攝影機傳送執行緒
        threading.Thread(target=camera_sender, daemon=True).start()

        while True:
            time.sleep(1)

    except KeyboardInterrupt:
        print("\n🛑 結束傳送")