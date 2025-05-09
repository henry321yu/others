import socket
import struct

UDP_IP = "0.0.0.0"
DIFOP_PORT = 2369

def parse_difop_data(data):
    def get_u16(offset): return struct.unpack_from(">H", data, offset)[0]
    def get_i16(offset): return struct.unpack_from(">h", data, offset)[0]
    def get_u8(offset): return data[offset]
    def get_u32(offset): return struct.unpack_from(">I", data, offset)[0]

    result = {
        "測距板溫度": get_i16(78) / 100.0,
        "APD 溫度": get_i16(80) / 100.0,
        "LD 溫度": get_i16(82) / 100.0,
        "三維加速度X": get_i16(84),
        "三維加速度Y": get_i16(86),
        "三維加速度Z": get_i16(88),
        "三軸角速度X": get_i16(90),
        "三軸角速度Y": get_i16(92),
        "三軸角速度Z": get_i16(94),
        "FPS 轉速": get_u16(96),
        "內參版本號": get_u8(98),
        "外參版本號": get_u8(99),
        "外參模式": get_u8(100),
        "外參模式值": get_u8(101),
        "升級狀態": get_u8(102),
        "主控版本": get_u8(103),
    }

    return result

def listen_difop():
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.bind((UDP_IP, DIFOP_PORT))
    print(f"[DIFOP] Listening on UDP port {DIFOP_PORT}...")

    while True:
        data, _ = sock.recvfrom(1500)
        if len(data) >= 106:
            parsed = parse_difop_data(data)
            print("------ DIFOP 狀態 ------")
            for key, val in parsed.items():
                print(f"{key}: {val}")
            print("------------------------\n")

if __name__ == "__main__":
    listen_difop()
