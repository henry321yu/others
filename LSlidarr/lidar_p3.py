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
PACKET_SIZE = 1212  # 根據手冊，應為 1200 data + 6 extra info
DISTANCE_RESOLUTION = 0.0025  # 0.25 cm = 0.0025 m

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

# 設定 intensity 顏色的最大值閥值
MAX_INTENSITY = 128

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

        for firing in range(2):  # 每個 block 有兩組 16 通道
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

    # 解析封包最後的 Timestamp 與 Factory
    timestamp = struct.unpack_from("<I", data, 1200)[0]
    factory = struct.unpack_from("<H", data, 1204)[0]

    return points, intensities, timestamp, factory

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
    while True:
        data = packet_queue.get()
        points, intensities, timestamp, factory = parse_packet(data)
        if len(points) > 0:
            data_list.extend(points)
            intensity_list.extend(intensities)
            count += len(points)

        if len(data_list) >= max_points:
            elapsed = time.perf_counter() - start_time
            f = count / elapsed
            fp = max_points / f
            current_time = time.strftime("%H:%M:%S")

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
                max_points = 120000

            print(f"{max_points} points in {fp:.2f}s | {current_time} | {f/1000:.2f}k Hz")
            print(f"First point: ({np_points[0,0]:.3f}, {np_points[0,1]:.3f}, {np_points[0,2]:.3f})")
            print(f"Timestamp: {timestamp} | Factory: 0x{factory:04X}\n")

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
