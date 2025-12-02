import socket

# -----------------------------
# 本機綁定設定
# -----------------------------
LISTEN_IP = "0.0.0.0"  # 監聽所有網路介面
LISTEN_PORT = 5300

# -----------------------------
# 建立 UDP Socket
# -----------------------------
sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sock.bind((LISTEN_IP, LISTEN_PORT))

print(f"UDP Server listening on {LISTEN_IP}:{LISTEN_PORT}")

# -----------------------------
# 持續接收封包
# -----------------------------
while True:
    data, addr = sock.recvfrom(1024)  # 最多 1024 bytes
    re_data = data.decode()
    print(f"{addr}:{re_data}",end='')
