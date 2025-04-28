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
    global start_time_ns, gotnp, i_max_points, frame_count, dframe_count, ax, ay, az, tem,printclock
    while True:
        data = packet_queue.get()
        points, intensities = parse_packet(data)

        if len(points) > 0:
            data_list.extend(points)
            intensity_list.extend(intensities)

        if len(data_list) >= i_max_points:
            current_time = time.strftime("%H:%M:%S")
            current_time_ns = time.perf_counter_ns()
            elapsed_ns = current_time_ns - start_time_ns
            elapsed_s = elapsed_ns / 1e9
            frame_count += len(data_list)
            dframe_count += 1
            dfrequency = dframe_count / elapsed_s
            frequency = frame_count / elapsed_s / 1000

            for i, point in enumerate(points):
                azimuth = math.degrees(math.atan2(point[1], point[0]))  # 反推方位角
                distance = math.sqrt(point[0]**2 + point[1]**2 + point[2]**2)
                vert_angle = VERTICAL_ANGLES[i % len(VERTICAL_ANGLES)]  # 使用垂直角

                # 印出 LiDAR 資料以及接收到的加速度和溫度數據
                # print(f"{current_time}, {elapsed_s:.3f}s, {vert_angle},{azimuth:.2f},{distance:.2f},{ax},{ay},{az},{tem},Points:{len(data_list)}, {frequency:.2f} kHz, {dfrequency:.2f} Hz")
                if(elapsed_s > printclock):
                    print(f"{current_time}, {elapsed_s:.3f}s, {vert_angle},{azimuth:.2f},{distance:.2f},{ax},{ay},{az},{tem},Points:{len(data_list)}, {frequency:.2f} kHz, {dfrequency:.2f} Hz")
                    printclock = printclock + 1

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
    threading.Thread(target=monitor, daemon=True).start()
    threading.Thread(target=udp_data_receiver, daemon=True).start()

    # 🧠 這裡維持在主執行緒
    while True:
        time.sleep(0.0001)

if __name__ == "__main__":
    main()
