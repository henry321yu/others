import socket
import struct
import math
import time
import threading
import queue
import numpy as np
import open3d as o3d

# LiDAR 網路設定
UDP_IP = "0.0.0.0"
UDP_PORT = 2368
PACKET_SIZE = 1212
DISTANCE_RESOLUTION = 0.4  # 單位公尺

VERTICAL_ANGLES = [
    -16, 0, -14, 2, -12, 4, -10, 6,
    -8, 8, -6, 10, -4, 12, -2, 14
]

packet_queue = queue.Queue(maxsize=1000)

# Open3D 設定
pcd = o3d.geometry.PointCloud()
vis = o3d.visualization.Visualizer()
vis.create_window(window_name="Real-time 3D LiDAR Point Cloud", width=800, height=600)
vis.add_geometry(pcd)
render_option = vis.get_render_option()
render_option.point_size = 2.0

data_list = []
intensity_list = []
max_points = 320000
count = 0
start_time = time.perf_counter()
gotnp = 0

# intensity 顏色映射最大值
MAX_INTENSITY = 100

# 頻率統計變數
data_count_lock = threading.Lock()
per_second_counter = 0

def parse_packet(data):
    blocks = 12
    channels = 16
    points = []
    intensities = []

    for block_idx in range(blocks):
        base = 100 * block_idx
        if data[base:base+2] != b'\xFF\xEE':
            continue
        azimuth = struct.unpack_from("<H", data, base + 2)[0] / 100.0
        azimuth_rad = math.radians(azimuth)

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
                vert_rad = math.radians(vert_angle)

                x = distance * math.cos(vert_rad) * math.sin(azimuth_rad)
                y = distance * math.cos(vert_rad) * math.cos(azimuth_rad)
                z = distance * math.sin(vert_rad)

                points.append([x, y, z])
                intensities.append(intensity)

    return points, intensities

def receiver_thread(sock):
    while True:
        data, _ = sock.recvfrom(PACKET_SIZE)
        if len(data) == PACKET_SIZE:
            try:
                packet_queue.put_nowait(data)
            except queue.Full:
                pass

def plotter_thread():
    global count, start_time, gotnp, max_points
    FREQ_INTERVAL = 5.0  # 每 2 秒顯示一次頻率資訊
    last_time = time.perf_counter()
    freq_point_counter = 0

    while True:
        data = packet_queue.get()
        points, intensities = parse_packet(data)
        if len(points) > 0:
            data_list.extend(points)
            intensity_list.extend(intensities)
            count += len(points)
            freq_point_counter += len(points)

        current = time.perf_counter()
        if current - last_time >= FREQ_INTERVAL:
            elapsed = current - last_time
            points_per_sec = freq_point_counter / elapsed
            current_time = time.strftime("%H:%M:%S")
            print(f"[{current_time}] {freq_point_counter} points in {elapsed:.2f}s | {points_per_sec/1000:.2f}k Hz")
            last_time = current
            freq_point_counter = 0

        if len(data_list) >= max_points:
            np_points = np.array(data_list, dtype=np.float32)
            np_intensity = np.array(intensity_list, dtype=np.float32)

            # 使用 MAX_INTENSITY 來設定顏色最大值
            norm_intensity = (np_intensity - np_intensity.min()) / (min(MAX_INTENSITY, np.ptp(np_intensity)) + 1e-6)

            # 彩色漸變映射
            colors = np.zeros((len(norm_intensity), 3), dtype=np.float32)
            for i, val in enumerate(norm_intensity):
                if val < 0.5:
                    t = val * 2
                    colors[i] = [0, t, 1 - t]
                else:
                    t = (val - 0.5) * 2
                    colors[i] = [t, 0, 1 - t]

            pcd.points = o3d.utility.Vector3dVector(np_points)
            pcd.colors = o3d.utility.Vector3dVector(colors)

            vis.update_geometry(pcd)
            vis.poll_events()
            vis.update_renderer()

            if gotnp != 1:
                vis.reset_view_point(True)
                vc = vis.get_view_control()
                vc.set_lookat([0, 0, 0])
                vc.set_zoom(0.75)
                gotnp = 1
                max_points = 32000

            # 清除暫存並重置
            data_list.clear()
            intensity_list.clear()
            count = 0
            start_time = time.perf_counter()

def main():
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.bind((UDP_IP, UDP_PORT))
    print(f"Listening on UDP port {UDP_PORT}...")

    threading.Thread(target=receiver_thread, args=(sock,), daemon=True).start()
    plotter_thread()

if __name__ == "__main__":
    main()
