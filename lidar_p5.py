import socket
import struct
import math
import time
import threading
import queue
import numpy as np
import open3d as o3d

# LiDAR 設定
UDP_IP = "0.0.0.0"
UDP_PORT = 2368
PACKET_SIZE = 1212
DISTANCE_RESOLUTION = 0.004

VERTICAL_ANGLES = [
    -16, 0, -14, 2, -12, 4, -10, 6,
    -8, 8, -6, 10, -4, 12, -2, 14
]

packet_queue = queue.Queue(maxsize=1000)
pcd = o3d.geometry.PointCloud()
data_list = []
intensity_list = []
max_points = 160000
count = 0
start_time = time.perf_counter()
gotnp = 0

# 建立視窗
vis = o3d.visualization.Visualizer()
vis.create_window("Real-time 3D LiDAR Point Cloud", width=800, height=600)
vis.add_geometry(pcd)
render_option = vis.get_render_option()
render_option.point_size = 2.0

# 畫格線
def draw_grid_lines(grid_size=1, extent=15): #設定網格長度
    lines = []
    points = []
    for i in range(-extent, extent + 1):
        points.append([i * grid_size, -extent * grid_size, 0])
        points.append([i * grid_size, extent * grid_size, 0])
        lines.append([len(points) - 2, len(points) - 1])
        points.append([-extent * grid_size, i * grid_size, 0])
        points.append([extent * grid_size, i * grid_size, 0])
        lines.append([len(points) - 2, len(points) - 1])
    line_set = o3d.geometry.LineSet()
    line_set.points = o3d.utility.Vector3dVector(points)
    line_set.lines = o3d.utility.Vector2iVector(lines)
    line_set.colors = o3d.utility.Vector3dVector([[0.7, 0.7, 0.7]] * len(lines))
    vis.add_geometry(line_set)

draw_grid_lines()

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
        for ch in range(channels):
            offset = base + 4 + ch * 3
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

def pointcloud_updater():
    global count, start_time, gotnp, max_points
    while True:
        data = packet_queue.get()
        points, intensities = parse_packet(data)
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

            if np_intensity.max() > 0:
                norm_intensity = np.clip(np_intensity / 255, 0, 1)   #調整顏色閥值
            else:
                norm_intensity = np.zeros_like(np_intensity)

            colors = np.zeros((len(norm_intensity), 3), dtype=np.float32)
            for i, val in enumerate(norm_intensity):
                if val < 0.25:
                    t = val * 4
                    colors[i] = [0.0, t, 1.0]
                elif val < 0.5:
                    t = (val - 0.25) * 4
                    colors[i] = [t, 1.0, 0.0]
                elif val < 1.0:
                    t = (val - 0.5) * 2
                    colors[i] = [1.0, 1.0 - t, 0.0]
                else:
                    colors[i] = [1.0, 0.0, 0.0]

            new_pcd = o3d.geometry.PointCloud()
            new_pcd.points = o3d.utility.Vector3dVector(np_points)
            new_pcd.colors = o3d.utility.Vector3dVector(colors)

            pcd.points = new_pcd.points
            pcd.colors = new_pcd.colors

            if gotnp != 1:
                vc = vis.get_view_control()
                vc.set_lookat([0, 0, 0])
                vc.set_zoom(0.5)
                gotnp = 1
                max_points = 64000

            data_list.clear()
            intensity_list.clear()
            count = 0
            start_time = time.perf_counter()
            print(f"{max_points} points in {fp:.2f}s | {current_time} | {f/1000:.2f}k Hz")

def main():
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.bind((UDP_IP, UDP_PORT))
    print(f"Listening on UDP port {UDP_PORT}...")

    threading.Thread(target=receiver_thread, args=(sock,), daemon=True).start()
    threading.Thread(target=pointcloud_updater, daemon=True).start()

    # 🧠 這裡維持在主執行緒
    while True:
        vis.poll_events()
        vis.update_renderer()
        vis.update_geometry(pcd)  # 強制更新 PointCloud
        time.sleep(0.01)

if __name__ == "__main__":
    main()
