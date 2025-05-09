import socket
import struct

UDP_IP = "0.0.0.0"
DIFOP_PORT = 2369

# 檢查 Header 與 Tail
EXPECTED_HEADER = b'\xA5\xFF\x00\x5A\x11\x11\x55\x55'
EXPECTED_TAIL = b'\x0F\xF0'

def parse_difop_packet(data):
    if len(data) != 1248:
        print("Invalid packet length:", len(data))
        return

    if not data.startswith(EXPECTED_HEADER) or not data.endswith(EXPECTED_TAIL):
        print("Invalid Header or Tail")
        return

    # 擷取欄位
    motor_speed = struct.unpack_from('>H', data, 8)[0]
    ip_port_mac = data[10:32].hex()
    gateway_subnet = data[32:40].hex()
    rotation_mode = struct.unpack_from('>H', data, 40)[0]
    flow_packet_interval = data[42:50].hex()
    pps_align_angle = struct.unpack_from('>H', data, 48)[0]
    pps_deviation = struct.unpack_from('>H', data, 50)[0]

    # UTC Time: 年/月/日/時/分/秒
    utc = struct.unpack_from('>6B', data, 52)
    utc_str = f"{2000+utc[0]:04d}-{utc[1]:02d}-{utc[2]:02d} {utc[3]:02d}:{utc[4]:02d}:{utc[5]:02d}"

    # GPS (22 bytes)：先印 hex
    gps_hex = data[58:80].hex()

    # 溫度（2 bytes signed short）除以 100 轉 °C
    temp = struct.unpack_from('>h', data, 80)[0] / 100.0

    # 垂直角度（32 bytes）先印 hex
    vertical_angles = data[245:277].hex()

    # 序號
    serial_number = data[1164:1184].decode(errors='ignore')

    # 版本
    version = struct.unpack_from('>H', data, 1196)[0]

    # 印出資訊
    print(f"\n========== DIFOP Packet ==========")
    print(f"Motor Speed            : {motor_speed}")
    print(f"Rotation Mode          : {rotation_mode}")
    print(f"Flow Packet Interval   : {flow_packet_interval}")
    print(f"PPS Align Angle        : {pps_align_angle}")
    print(f"PPS Deviation          : {pps_deviation}")
    print(f"UTC Time               : {utc_str}")
    print(f"GPS Hex                : {gps_hex}")
    print(f"Temp (Plate 2)         : {temp:.2f} °C")
    print(f"Vertical Angles Hex    : {vertical_angles}")
    print(f"Serial Number          : {serial_number.strip()}")
    print(f"Firmware Version       : {version}")
    print(f"MAC/IP/Port (raw hex)  : {ip_port_mac}")
    print(f"Gateway/Subnet (hex)   : {gateway_subnet}")
    print("====================================\n")

# 建立 socket 並接收 DIFOP 封包
sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sock.bind((UDP_IP, DIFOP_PORT))
print(f"Listening for DIFOP packets on UDP port {DIFOP_PORT}...")

try:
    while True:
        data, addr = sock.recvfrom(2048)
        parse_difop_packet(data)

except KeyboardInterrupt:
    print("Terminated by user.")

finally:
    sock.close()
