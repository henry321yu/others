import socket
import struct
import math
import time
import threading
import queue

# LiDAR 設定
UDP_IP = "0.0.0.0"
UDP_PORT = 2368
PACKET_SIZE = 1212
DISTANCE_RESOLUTION = 0.004

VERTICAL_ANGLES = [
    -15, 1, -13, 3, -11, 5, -9, 7,
    -7, 9, -5, 11, -3, 13, -1, 15
]

packet_queue = queue.Queue(maxsize=1000)

# 記錄用
frame_count = 0
start_time_ns = time.perf_counter_ns()
max_points = 320000
data_points = []

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
    while True:
        data, _ = sock.recvfrom(PACKET_SIZE)
        if len(data) == PACKET_SIZE:
            try:
                packet_queue.put_nowait(data)
            except queue.Full:
                pass

def pointcloud_logger():
    global frame_count, start_time_ns, data_points
    while True:
        data = packet_queue.get()
        parsed = parse_packet(data)
        if parsed:
            data_points.extend(parsed)

        if len(data_points) >= max_points:
            current_time_ns = time.perf_counter_ns()
            elapsed_ns = current_time_ns - start_time_ns
            elapsed_s = elapsed_ns / 1e9
            frame_count += len(data_points)
            frequency = frame_count / elapsed_s/ 1000
            print(f"{elapsed_s:3f}s,Frame:{frame_count},Points:{len(data_points)},{frequency:.2f} kHz")
            # for v_angle, azimuth, distance, intensity in data_points:
            #     print(f"{elapsed_s:6f} s,{v_angle:.2f}, {azimuth:.2f}, {distance:.3f}, {intensity},{frequency:.2f} Hz")

            # 清除資料準備下一幀
            data_points.clear()

def main():
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.bind((UDP_IP, UDP_PORT))
    print(f"Listening on UDP port {UDP_PORT}...")

    threading.Thread(target=receiver_thread, args=(sock,), daemon=True).start()
    threading.Thread(target=pointcloud_logger, daemon=True).start()

    while True:
        time.sleep(1)

if __name__ == "__main__":
    main()
