import socket
import struct
import math
import time
import threading
import queue

UDP_IP = "0.0.0.0"
UDP_PORT = 2368
PACKET_SIZE = 1212
DISTANCE_RESOLUTION = 0.4  # meters

VERTICAL_ANGLES = [
    -16, 0, -14, 2, -12, 4, -10, 6,
    -8, 8, -6, 10, -4, 12, -2, 14
]

start_time = time.time()
packet_queue = queue.Queue(maxsize=1000)  # 大一點避免丟資料

def parse_packet(data):
    output_lines = []
    blocks = 12
    channels = 16
    current_time = time.time() - start_time

    for block_idx in range(blocks):
        base = 100 * block_idx
        if data[base:base+2] != b'\xFF\xEE':
            continue
        azimuth = struct.unpack_from("<H", data, base + 2)[0] / 100.0
        for ch in range(channels):
            offset = base + 4 + ch * 3
            distance_raw = struct.unpack_from("<H", data, offset)[0]
            intensity = data[offset + 2]
            distance = distance_raw * DISTANCE_RESOLUTION
            if distance == 0:
                continue
            vert_angle = VERTICAL_ANGLES[ch]
            output_lines.append(f"{current_time:.3f},{azimuth:.2f},{vert_angle},{distance:.3f},{intensity}")
    return output_lines

def receiver_thread(sock):
    while True:
        data, _ = sock.recvfrom(PACKET_SIZE)
        if len(data) == PACKET_SIZE:
            try:
                packet_queue.put_nowait(data)
            except queue.Full:
                pass  # 佇列滿就跳過，避免阻塞

def writer_thread():
    with open("lidar_output.csv", "w") as f:
        f.write("time,azimuth,vert_angle,distance,intensity\n")
        while True:
            data = packet_queue.get()
            lines = parse_packet(data)
            if lines:
                f.write("\n".join(lines) + "\n")

def main():
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.bind((UDP_IP, UDP_PORT))
    print(f"Listening on UDP port {UDP_PORT}...")

    threading.Thread(target=receiver_thread, args=(sock,), daemon=True).start()
    writer_thread()  # 主執行緒負責寫檔

if __name__ == "__main__":
    main()
