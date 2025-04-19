import socket
import struct
import math
import time

UDP_IP = "0.0.0.0"
UDP_PORT = 2368
PACKET_SIZE = 1212
DISTANCE_RESOLUTION = 0.4  # 公分為單位

VERTICAL_ANGLES = [
    -16, 0, -14, 2, -12, 4, -10, 6,
    -8, 8, -6, 10, -4, 12, -2, 14
]

start_time = time.time()  # 程式啟動時間

# 統計輸出行數
line_counter = 0
last_print_time = start_time

def parse_packet(data):
    global line_counter
    blocks = 12
    channels = 16
    current_time = time.time() - start_time

    output_lines = []

    for block_idx in range(blocks):
        base = 100 * block_idx
        header = data[base:base+2]
        if header != b'\xFF\xEE':
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
            line_counter += 1

    # if output_lines:
    #     print("\n".join(output_lines))

def main():
    global line_counter, last_print_time
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.bind((UDP_IP, UDP_PORT))
    print(f"Listening on UDP port {UDP_PORT}...")

    while True:
        data, _ = sock.recvfrom(PACKET_SIZE)
        if len(data) == PACKET_SIZE:
            parse_packet(data)

        # 每秒顯示一次輸出頻率
        current_time = time.time()
        if current_time - last_print_time >= 1.0:
            print(f"[INFO] Output frequency: {line_counter} Hz")
            line_counter = 0
            last_print_time = current_time

if __name__ == "__main__":
    main()
