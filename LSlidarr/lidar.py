import socket
import struct
import math
import time

UDP_IP = "0.0.0.0"
UDP_PORT = 2368
# PACKET_SIZE = 1206
PACKET_SIZE = 1212

DISTANCE_RESOLUTION = 0.4  # 0.004 meter = 0.4 cm

# 垂直角度表（LS C16 固定16線）
VERTICAL_ANGLES = [
    -16, 0, -14, 2, -12, 4, -10, 6,
    -8, 8, -6, 10, -4, 12, -2, 14
]

start_time = time.time()  # 程式啟動時間

def parse_packet(data):
    blocks = 12
    channels = 16
    current_time = time.time() - start_time  # 相對啟動時間（秒）
    for block_idx in range(blocks):
        base = 100 * block_idx
        header = data[base:base+2]
        if header != b'\xFF\xEE':
            continue  # skip invalid blocks

        azimuth = struct.unpack_from("<H", data, base + 2)[0] / 100.0  # 0.01 deg units
        for ch in range(channels):
            offset = base + 4 + ch * 3
            distance_raw = struct.unpack_from("<H", data, offset)[0]
            intensity = data[offset + 2]
            distance = distance_raw * DISTANCE_RESOLUTION

            if distance == 0:
                continue  # 跳過無效資料

            vert_angle = VERTICAL_ANGLES[ch]

            print(f"{current_time:.3f},{azimuth:.2f},{vert_angle},{distance:.3f},{intensity}")

def main():
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.bind((UDP_IP, UDP_PORT))
    print(f"Listening on UDP port {UDP_PORT}...")

    while True:
        data, _ = sock.recvfrom(PACKET_SIZE)
        if len(data) == PACKET_SIZE:
            parse_packet(data)

if __name__ == "__main__":
    main()

