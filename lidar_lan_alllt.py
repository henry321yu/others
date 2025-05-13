import socket
import time
import os
import platform
import threading
import smbus2

# --------- LiDAR 參數與 Socket ---------
LIDAR_PORTS = [2368, 2369]
LIDAR_IP = ''  # 綁定所有介面

REMOTE_PC_LIST = [
    '10.241.0.114',
    '10.241.215.99',
    '10.241.180.148',
    '10.241.199.211',
    '10.241.183.64',
    '10.241.66.133',
    # 更多 IP...
]

# --------- ADXL355 參數 ---------
ACCPORT = 2370
I2C_ADDR = 0x1D
bus = smbus2.SMBus(1)

TEMP2 = 0x06
RESET = 0x2F
POWER_CTL = 0x2D
RANGE = 0x2C
SELF_TEST = 0x2E

def write_355(addr, value):
    bus.write_byte_data(I2C_ADDR, addr, value)

def setup_355():
    write_355(RESET, 0x52)
    time.sleep(0.1)
    write_355(POWER_CTL, 0x00)
    time.sleep(0.03)
    write_355(RANGE, 0x01)
    time.sleep(0.03)
    write_355(SELF_TEST, 0x00)
    time.sleep(0.1)

def read_355():
    var = bus.read_i2c_block_data(I2C_ADDR, TEMP2, 11)
    ax = (var[2] << 12 | var[3] << 4 | var[4] >> 4)
    ay = (var[5] << 12 | var[6] << 4 | var[7] >> 4)
    az = (var[8] << 12 | var[9] << 4 | var[10] >> 4)
    if ax > 0x80000: ax -= 0x100000
    if ay > 0x80000: ay -= 0x100000
    if az > 0x80000: az -= 0x100000
    ax /= 0x3E800
    ay /= 0x3E800
    az /= 0x3E800
    temp_raw = (var[0] << 8 | var[1])
    temp = ((1852 - temp_raw) / 9.05) + 27.2
    return ax, ay, az, temp

# --------- socket 設定 ---------
sock_lidar = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sock_lidar.bind((LIDAR_IP, LIDAR_PORTS[0]))

sock_lidar2 = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sock_lidar2.bind((LIDAR_IP, LIDAR_PORTS[1]))

sock_acc = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

# --------- 狀態追蹤 ---------
packet_count_lidar = [0, 0]
total_bytes_sent_lidar = [0, 0]
total_bytes_sent_acc = 0

sent_bytes_lidar = [{ip: 0 for ip in REMOTE_PC_LIST} for _ in range(2)]
sent_bytes_acc = {ip: 0 for ip in REMOTE_PC_LIST}
online_status = {ip: False for ip in REMOTE_PC_LIST}

# --------- 系統工具 ---------
def clear_screen():
    os.system('cls' if platform.system().lower() == 'windows' else 'clear')

def ping(ip):
    param = "-n" if platform.system().lower() == "windows" else "-c"
    return os.system(f"ping {param} 1 {ip} > /dev/null 2>&1") == 0

def ping_checker():
    while True:
        for ip in REMOTE_PC_LIST:
            online_status[ip] = ping(ip)
        time.sleep(5)

def display_status():
    clear_screen()
    print("LIDAR 傳輸狀態：")
    for i, port in enumerate(LIDAR_PORTS):
        print(f"  Port {port} 封包數：{packet_count_lidar[i]}")
        print(f"  傳輸量：{total_bytes_sent_lidar[i] / (1024*1024):.2f} MB")
    print(f"ADXL355 傳輸量：{total_bytes_sent_acc / (1024*1024):.2f} MB")
    print("各電腦狀態：")
    for ip in REMOTE_PC_LIST:
        status = "online" if online_status[ip] else "offline"
        mb_lidar0 = sent_bytes_lidar[0][ip] / (1024 * 1024)
        mb_lidar1 = sent_bytes_lidar[1][ip] / (1024 * 1024)
        mb_acc = sent_bytes_acc[ip] / (1024 * 1024)
        print(f"  {ip}: 2368={mb_lidar0:.2f}MB | 2369={mb_lidar1:.2f}MB | 2370={mb_acc:.2f}MB  {status}")
    print("-" * 40)

# --------- 後台執行緒 ---------
threading.Thread(target=ping_checker, daemon=True).start()
setup_355()

def lidar_receiver(sock, port_index):
    while True:
        try:
            data, _ = sock.recvfrom(1500)
        except Exception:
            continue
        for ip in REMOTE_PC_LIST:
            if not online_status[ip]: continue
            try:
                sock.sendto(data, (ip, LIDAR_PORTS[port_index]))
                total_bytes_sent_lidar[port_index] += len(data)
                sent_bytes_lidar[port_index][ip] += len(data)
            except:
                pass
        packet_count_lidar[port_index] += 1

# 開啟兩個執行緒分別處理 2368 和 2369
threading.Thread(target=lidar_receiver, args=(sock_lidar, 0), daemon=True).start()
threading.Thread(target=lidar_receiver, args=(sock_lidar2, 1), daemon=True).start()

# --------- 主執行緒：傳送 ADXL355 ---------
last_display_time = time.time()

while True:
    ax, ay, az, temp = read_355()
    message = f"ADXL355,{ax:.6f},{ay:.6f},{az:.6f},{temp:.2f}"
    data = message.encode('utf-8')

    for ip in REMOTE_PC_LIST:
        if not online_status[ip]: continue
        try:
            sock_acc.sendto(data, (ip, ACCPORT))
            total_bytes_sent_acc += len(data)
            sent_bytes_acc[ip] += len(data)
        except:
            pass

    if time.time() - last_display_time >= 1:
        display_status()
        last_display_time = time.time()

    time.sleep(0.01)  # 每 10ms 傳送 ADXL355
