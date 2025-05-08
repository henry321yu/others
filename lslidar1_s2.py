import socket
import struct
import math
import time
import threading
import queue
import os
from datetime import datetime

# LiDAR 設定
UDP_IP = "0.0.0.0"
UDP_PORT = 2368
PACKET_SIZE = 1212
DISTANCE_RESOLUTION = 0.004

VERTICAL_ANGLES = [
    -15, 1, -13, 3, -11, 5, -9, 7,
    -7, 9, -5, 11, -3, 13, -1, 15
]

data_list = []
intensity_list = []
i_max_points = 16000  # 20hz
frame_count = 0
dframe_count = 0
start_time_ns = time.perf_counter_ns()
gotnp = 0
packet_queue = queue.Queue(maxsize=i_max_points)
packet_queue_full = 0

# 接收設定
LOCAL_IP = "0.0.0.0"
LOCAL_PORT = 2370  # 必須跟送端一樣

# 建立UDP socket
sock_recv = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sock_recv.bind((LOCAL_IP, LOCAL_PORT))

# 新增全局變數
ax = ay = az = tem = 0.0  # 用來存放接收到的加速度和溫度數據
printclock = 0

#save data
log_queue = queue.Queue(maxsize=i_max_points)
log_queue_full=0

import os

def file_logger():
    base_name = "lslidar_output"
    extension = ".txt"    
    tt=0

    # 偵測當前資料夾的最大 file_index
    def get_existing_max_index():
        max_index = -1
        for file_name in os.listdir():
            if file_name.startswith(base_name) and file_name.endswith(extension):
                try:
                    index = int(file_name[len(base_name)+1:-len(extension)])  # 提取索引部分
                    if index > max_index:
                        max_index = index
                except ValueError:
                    continue
        return max_index

    # 初始檔案索引
    file_index = get_existing_max_index() + 1 if get_existing_max_index() != -1 else 0

    def get_file_path(index):
        return f"{base_name}_{index:02d}{extension}"

    # 創建第一個檔案
    current_file = open(get_file_path(file_index), "w")
    current_file.write("time,program_time,vert_angle,azimuth,distance,ax,ay,az,temperture,points,frequency(khz),program_frequency\n")

    while True:
        try:
            line = log_queue.get()
            if line:
                parts = line.split(",")
                if len(parts) > 1:
                    program_time = float(parts[1])
                    if program_time-tt > 86400:
                        tt=program_time
                        current_file.close()
                        file_index = (file_index + 1) % 100  # 確保在 00~99 範圍循環
                        current_file = open(get_file_path(file_index), "w")
                        current_file.write("time,program_time,vert_angle,azimuth,distance,ax,ay,az,temperture,points,frequency(khz),program_frequency\n")
                current_file.write(line + "\n")
                current_file.flush()
            log_queue.task_done()
        except Exception as e:
            print(f"[Logger Error] {e}")


def parse_packet(data):
    blocks = 12
    channels = 16
    results = []

    for block_idx in range(blocks):
        base = 100 * block_idx
        if data[base:base+2] != b'\xFF\xEE':
            continue
        azimuth = struct.unpack_from("<H", data, base + 2)[0] / 100.0
        for firing in range(2):
            for ch in range(channels):
                idx = firing * channels + ch
                offset = base + 4 + idx * 3
                distance_raw = struct.unpack_from("<H", data, offset)[0]
                intensity = data[offset + 2]
                distance = distance_raw * DISTANCE_RESOLUTION
                if distance == 0:
                    continue
                vert_angle = VERTICAL_ANGLES[ch]

                results.append((vert_angle, azimuth, distance, intensity))
    return results

def receiver_thread(sock):
    global packet_queue_full
    while True:
        data, _ = sock.recvfrom(PACKET_SIZE)
        if len(data) == PACKET_SIZE:
            try:
                packet_queue.put_nowait(data)
            except queue.Full:                
                packet_queue_full = 1
                pass

def pointcloud_updater():
    global start_time_ns, gotnp, i_max_points, frame_count, dframe_count, ax, ay, az, tem, printclock
    while True:
        data = packet_queue.get()
        points = parse_packet(data)
        if len(points) > 0:
            data_list.extend(points)
        if len(data_list) >= i_max_points:            
            current_time = datetime.now().strftime("%H:%M:%S.%f")[:-4]  # 精確到百分之一秒（兩位小數）
            current_time_ns = time.perf_counter_ns()
            elapsed_ns = current_time_ns - start_time_ns
            elapsed_s = elapsed_ns / 1e9
            frame_count += len(data_list)
            dframe_count += 1
            dfrequency = dframe_count / elapsed_s
            frequency = frame_count / elapsed_s / 1000

            for vert_angle, azimuth, distance, intensity in data_list:

                # 印出 LiDAR 資料以及接收到的加速度和溫度數據
                # print(f"{current_time}, {elapsed_s:.3f}s, {vert_angle}, {azimuth:.2f}, {distance:.2f}, {ax}, {ay}, {az}, {tem}°C, Points:{len(data_list)}, {frequency:.2f} kHz, {dfrequency:.2f} Hz")
                if(elapsed_s > printclock):
                    printclock = printclock + 0.5
                    print(f"{current_time}, {elapsed_s:.3f}s, {vert_angle}, {azimuth:.2f}, {distance:.2f}, {ax}, {ay}, {az}, {tem}°C, Points:{len(data_list)}, {frequency:.2f} kHz, {dfrequency:.2f} Hz")
                    log_line = f"{current_time},{elapsed_s:.3f},{vert_angle},{azimuth:.2f},{distance:.2f},{ax},{ay},{az},{tem},{len(data_list)},{frequency:.2f},{dfrequency:.2f}"
                    log_queue.put_nowait(log_line)

            # 清空數據列表
            data_list.clear()
            intensity_list.clear()

def monitor():
    global packet_queue_full
    while True:        
        if packet_queue_full == 1:
            print(f"[Monitor] packet_queue is full!!")
        else:            
            print(f"[Monitor] packet_queue size: {packet_queue.qsize()}")
        packet_queue_full = 0
        time.sleep(2)

def udp_data_receiver():
    global ax, ay, az, tem
    while True:
        try:
            data, addr = sock_recv.recvfrom(1024)  # 最多收1024 bytes
            message = data.decode('utf-8')

            # 分割資料
            parts = message.split(',')
            if len(parts) == 5:
                ID = parts[0]
                ax = float(parts[1])
                ay = float(parts[2])
                az = float(parts[3])
                tem = float(parts[4])
        except Exception as e:
            pass

def main():
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.bind((UDP_IP, UDP_PORT))
    print(f"Listening on UDP port {UDP_PORT}...")

    threading.Thread(target=receiver_thread, args=(sock,), daemon=True).start()
    threading.Thread(target=pointcloud_updater, daemon=True).start()
    # threading.Thread(target=monitor, daemon=True).start()
    threading.Thread(target=udp_data_receiver, daemon=True).start()
    threading.Thread(target=file_logger, daemon=True).start()

    while True:
        time.sleep(1)

if __name__ == "__main__":
    main()
