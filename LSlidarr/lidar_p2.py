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
DISTANCE_RESOLUTION = 0.4  # meters

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
render_option.point_size = 2.0  # 可調整點大小

# 資料收集
data_list = []
intensity_list = []
max_points = 320000
count = 0
start_time = time.perf_counter()
gotnp = 0

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
            intensity = data[offset+2]
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

            # Normalize intensity (0~1)，避免除以0
            if np_intensity.max() > 0:
                norm_intensity = np.clip(np_intensity / np_intensity.max(), 0, 1)
            else:
                norm_intensity = np.zeros_like(np_intensity)

            # 轉換為 RGB 彩色（可改為更複雜 colormap）
            colors = np.stack([norm_intensity]*3, axis=1)  # 灰階 (R=G=B)

            if np_points.size > 0:
                pcd.points = o3d.utility.Vector3dVector(np_points)

                # ==== 彩色映射根據 intensity ====
                intensities_np = np.array(intensities, dtype=np.float32)
                norm_intensity = (intensities_np - intensities_np.min()) / (np.ptp(intensities_np) + 1e-6)

                # 漸變：綠（低）→ 藍（中）→ 紅（高）
                colors = np.zeros((len(norm_intensity), 3), dtype=np.float32)
                for i, val in enumerate(norm_intensity):
                    if val < 0.5:
                        # 綠 → 藍
                        t = val * 2  # 0~1
                        colors[i] = [0, t, 1 - t]  # G→B
                    else:
                        # 藍 → 紅
                        t = (val - 0.5) * 2  # 0~1
                        colors[i] = [t, 0, 1 - t]  # B→R

                pcd.colors = o3d.utility.Vector3dVector(colors)

                vis.update_geometry(pcd)
                vis.poll_events()
                vis.update_renderer()

                # ==== 視角設定（只跑一次）====
                if gotnp != 1:
                    vis.reset_view_point(True)
                    vc = vis.get_view_control()
                    vc.set_lookat([0, 0, 0])     # 看中心點
                    vc.set_front([0, 1, 0])     # 從 Y- 看過去
                    vc.set_up([0, 0, 1])         # Z 軸向上
                    vc.set_zoom(0.75)
                    gotnp = 1
                    max_points = 32000

            print(f"{max_points} points in {fp:.2f}s | {current_time} | {f/1000:.2f}k Hz | First point: ({np_points[0,0]:.3f}, {np_points[0,1]:.3f}, {np_points[0,2]:.3f})")

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
