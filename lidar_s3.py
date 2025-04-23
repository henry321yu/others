import socket
import struct
import time
import threading
import queue
import os  # 在開頭加上

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
max_points = 64000
data_points = []
# file_path = r"C:\Users\sgrc-325\Desktop\git\lidar\lslidarr\lidar_output.txt"
file_path = "lidar_output.txt"

log_queue = queue.Queue(maxsize=max_points*50)

def file_logger():
    with open(file_path, "w") as f:
        f.write("time,vert_angle,azimuth,distance,intensity,frequency\n")
        buffer = []
        while True:
            try:
                line = log_queue.get()
                buffer.append(line)
                log_queue.task_done()

                # 每 k 行寫一次
                if len(buffer) >= 100000:
                    f.write("\n".join(buffer) + "\n")
                    f.flush()
                    buffer.clear()
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
            frequency = frame_count / elapsed_s / 1000

            for v_angle, azimuth, distance, intensity in data_points:
                log_line = f"{elapsed_s:3f}, {v_angle:.2f}, {azimuth:.2f}, {distance:.3f}, {intensity}, {frequency:.2f}"
                try:
                    log_queue.put_nowait(log_line)
                except queue.Full:
                    print("queue.Full")
                    pass

            # 印出目前檔案大小、時間與頻率
            try:
                file_size = os.path.getsize(file_path)
            except FileNotFoundError:
                file_size = 0
            print(f"[{elapsed_s:.2f}s] File size: {file_size / 1048576:.2f} MB, Data length: {frame_count/1e6} M points, Frequency: {frequency:.2f} KHz")

            data_points.clear()

def monitor():
    while True:
        print(f"[Monitor] Queue size: {log_queue.qsize()}")
        time.sleep(2)


def main():
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.bind((UDP_IP, UDP_PORT))
    print(f"Listening on UDP port {UDP_PORT}...")

    threading.Thread(target=receiver_thread, args=(sock,), daemon=True).start()
    threading.Thread(target=pointcloud_logger, daemon=True).start()
    threading.Thread(target=file_logger, daemon=True).start()
    threading.Thread(target=monitor, daemon=True).start()

    while True:
        time.sleep(1)

if __name__ == "__main__":
    main()
