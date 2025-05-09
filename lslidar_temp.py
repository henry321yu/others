import socket
import struct
import time
import threading
import queue
from datetime import datetime

# 設定
UDP_IP = "0.0.0.0"
MSOP_PORT = 2368
DIFOP_PORT = 2369
PACKET_SIZE = 1212
DISTANCE_RESOLUTION = 0.004

VERTICAL_ANGLES = [
    -15, 1, -13, 3, -11, 5, -9, 7,
    -7, 9, -5, 11, -3, 13, -1, 15
]

data_list = []
i_max_points = 16000
frame_count = 0
dframe_count = 0
start_time_ns = time.perf_counter_ns()
packet_queue = queue.Queue(maxsize=i_max_points)
packet_queue_full = 0

# 全局溫度資訊（由 DIFOP 解析）
apd_temp = 0.0
ld_temp = 0.0

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

def msop_receiver():
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.bind((UDP_IP, MSOP_PORT))
    print(f"[MSOP] Listening on UDP port {MSOP_PORT}...")
    while True:
        data, _ = sock.recvfrom(PACKET_SIZE)
        if len(data) == PACKET_SIZE:
            try:
                packet_queue.put_nowait(data)
            except queue.Full:
                global packet_queue_full
                packet_queue_full = 1

def difop_receiver():
    global apd_temp, ld_temp
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.bind((UDP_IP, DIFOP_PORT))
    print(f"[DIFOP] Listening on UDP port {DIFOP_PORT}...")
    while True:
        data, _ = sock.recvfrom(1500)
        if len(data) >= 84:
            try:
                apd_temp = struct.unpack_from(">h", data, 80)[0] / 100.0
                ld_temp = struct.unpack_from(">h", data, 82)[0] / 100.0
            except:
                pass

def pointcloud_updater():
    global start_time_ns, frame_count, dframe_count, packet_queue_full
    printclock = 0
    while True:
        data = packet_queue.get()
        points = parse_packet(data)
        if len(points) > 0:
            data_list.extend(points)
        if len(data_list) >= i_max_points:
            current_time = datetime.now().strftime("%H:%M:%S.%f")[:-4]
            current_time_ns = time.perf_counter_ns()
            elapsed_ns = current_time_ns - start_time_ns
            elapsed_s = elapsed_ns / 1e9
            frame_count += len(data_list)
            dframe_count += 1
            dfrequency = dframe_count / elapsed_s
            frequency = frame_count / elapsed_s / 1000

            # 印出第一筆點雲 + 溫度狀態
            if elapsed_s > printclock:
                printclock += 0.5
                va, az, dist, inten = data_list[0]
                print(f"{current_time}, {elapsed_s:.3f}s, {va}, {az:.2f}, {dist:.2f}, "
                      f"APD:{apd_temp:.2f}°C, LD:{ld_temp:.2f}°C, "
                      f"Points:{len(data_list)}, {frequency:.2f} kHz, {dfrequency:.2f} Hz")
            data_list.clear()

def main():
    threading.Thread(target=msop_receiver, daemon=True).start()
    threading.Thread(target=difop_receiver, daemon=True).start()
    threading.Thread(target=pointcloud_updater, daemon=True).start()

    while True:
        time.sleep(1)

if __name__ == "__main__":
    main()
