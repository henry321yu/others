import socket
import struct
import math
import time
import threading
import queue
import matplotlib.pyplot as plt
import numpy as np

# LiDAR 網路設定
UDP_IP = "0.0.0.0"
UDP_PORT = 2368
PACKET_SIZE = 1212
DISTANCE_RESOLUTION = 0.4  # 公尺

VERTICAL_ANGLES = [
    -16, 0, -14, 2, -12, 4, -10, 6,
    -8, 8, -6, 10, -4, 12, -2, 14
]

packet_queue = queue.Queue(maxsize=1000)

# Matplotlib 設定
plt.ion()
fig, ax = plt.subplots()
sc = ax.scatter([], [], s=1, c='blue')
ax.set_xlim(-50, 50)  # 公尺，可調整
ax.set_ylim(-50, 50)
ax.set_aspect('equal')
ax.set_xlabel("X (m)")
ax.set_ylabel("Y (m)")
ax.set_title("Real-time LiDAR 2D Point Cloud")

# 即時資料存儲
data_list = []
max_points = 1600
count = 0
start_time = time.perf_counter()

def parse_packet(data):
    blocks = 12
    channels = 16
    points = []

    for block_idx in range(blocks):
        base = 100 * block_idx
        if data[base:base+2] != b'\xFF\xEE':
            continue
        azimuth = struct.unpack_from("<H", data, base + 2)[0] / 100.0
        azimuth_rad = math.radians(azimuth)

        for ch in range(channels):
            offset = base + 4 + ch * 3
            distance_raw = struct.unpack_from("<H", data, offset)[0]
            intensity = data[offset + 2]
            distance = distance_raw * DISTANCE_RESOLUTION
            if distance == 0:
                continue
            vert_angle = VERTICAL_ANGLES[ch]
            vert_rad = math.radians(vert_angle)

            # 將球座標轉為直角座標 (X, Y, Z)
            x = distance * math.cos(vert_rad) * math.sin(azimuth_rad)
            y = distance * math.cos(vert_rad) * math.cos(azimuth_rad)
            z = distance * math.sin(vert_rad)

            points.append((x, y))  # 僅繪製 XY 平面（如從天花板往下看）

    return points

def receiver_thread(sock):
    while True:
        data, _ = sock.recvfrom(PACKET_SIZE)
        if len(data) == PACKET_SIZE:
            try:
                packet_queue.put_nowait(data)
            except queue.Full:
                pass

def plotter_thread():
    global count, start_time
    while True:
        data = packet_queue.get()
        points = parse_packet(data)
        data_list.extend(points)
        count += len(points)

        if len(data_list) >= max_points:
            elapsed = time.perf_counter() - start_time
            f = count / elapsed
            fp = max_points / f
            current_time = time.strftime("%H:%M:%S")

            # 繪圖
            x_vals, y_vals = zip(*data_list)
            sc.set_offsets(np.c_[x_vals, y_vals])
            ax.set_title(f"{max_points} points in {fp:.2f}s\n{current_time} - {f:.2f} Hz")
            plt.draw()
            plt.pause(0.01)

            # 重置
            data_list.clear()
            count = 0
            start_time = time.perf_counter()

def main():
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.bind((UDP_IP, UDP_PORT))
    print(f"Listening on UDP port {UDP_PORT}...")

    threading.Thread(target=receiver_thread, args=(sock,), daemon=True).start()
    plotter_thread()  # 主線程做繪圖

if __name__ == "__main__":
    main()
