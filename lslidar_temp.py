import socket
import struct

# 設定
UDP_IP = "0.0.0.0"
DIFOP_PORT = 2369

def read_difop_temperature():
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.bind((UDP_IP, DIFOP_PORT))
    print(f"[DIFOP] Listening on UDP port {DIFOP_PORT}...")

    while True:
        data, _ = sock.recvfrom(1500)
        if len(data) >= 84:
            try:
                temp = struct.unpack_from(">h", data, 80)[0] / 100.0
                print(f"Received temperature: {temp:.2f} °C")
            except Exception as e:
                print(f"Failed to parse temperature: {e}")

if __name__ == "__main__":
    read_difop_temperature()