import cv2
import time
import socket
import os
import platform
import threading
import smbus2
from datetime import datetime

# ========= 網路與設備清單 =========
REMOTE_PC_LIST = [
    ('10.241.0.114', 0),
    ('10.241.215.99', 0),
    ('10.241.180.148', 0),
    ('10.241.199.211', 0),
    ('10.241.183.64', 0),
    ('10.241.66.133', 0),
]

LIDAR1_PORT = 2368

# ========= 資料傳送 =========
sock_lidar1 = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sock_lidar1.bind(("", LIDAR1_PORT))
lidar1_sent = 0

# ========= LiDAR 資料接收轉發 =========
def lidar1():
    global lidar1_sent
    while True:
        data, _ = sock_lidar1.recvfrom(1500)
        for ip, _ in REMOTE_PC_LIST:
            sock_lidar1.sendto(data, (ip, LIDAR1_PORT))
            lidar1_sent += 1

# ========= 主程式 =========
def print_status():
    global lidar1_sent
    while True:
        os.system('cls' if platform.system().lower() == 'windows' else 'clear')
        print(f"  系統狀態：")
        print(f"  LiDAR1 傳送數量: {lidar1_sent}")
        print(f"  --------------------")
        time.sleep(1)

if __name__ == "__main__":
    threading.Thread(target=lidar1, daemon=True).start()
    threading.Thread(target=print_status, daemon=True).start()

    print("系統啟動中... 按 Ctrl+C 結束。")
    try:
        while True:
            time.sleep(1)
    except KeyboardInterrupt:        
        sock_lidar1.close()
        print("已手動中斷")
