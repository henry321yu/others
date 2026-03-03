import socket
import threading
import time

# ========= 網路與設備清單 =========
REMOTE_PC_LIST = [
    # ('10.241.215.99', 0),   # dell
    # ('10.241.199.211', 0),  # fly office
    # ('10.241.183.64', 0),   # ipc
    # ('10.241.66.133', 0),   # broken
    # ('10.241.178.17', 0),   # my phone
    #('10.241.135.1', 0),    # my new office
    # ('10.241.194.18', 0),    # rasp2
    # ('26.253.227.19', 0),  # my pc RADMIN ###
    # ('26.107.7.251', 0),  # ipc
    ('26.55.45.86', 0),  # my new office
    # ('26.208.29.50', 0),  # my old office
    # ('26.214.58.255', 0),  # dell
]

# ========= 要監聽與轉發的 Port =========
FORWARD_PORTS = [2370, 2385, 2386, 2368, 2369]

# ========= UDP 封包轉發函式 =========
def forward_packets(listen_port):
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    sock.bind(('', listen_port))
    print(f"[啟動] 監聽 Port {listen_port} 中...")

    packet_count = 0
    total_bytes_sent = 0
    start_time = time.time()

    while True:
        try:
            data, addr = sock.recvfrom(65536)
        except ConnectionResetError as e:
            print(f"[錯誤] Port {listen_port} - 遠端主機重設連線: {e}")
            continue
        except Exception as e:
            print(f"[錯誤] Port {listen_port} - 接收錯誤: {e}")
            continue

        for ip, _ in REMOTE_PC_LIST:
            try:
                sent_bytes = sock.sendto(data, (ip, listen_port))
                total_bytes_sent += sent_bytes
            except Exception as e:
                print(f"[錯誤] 傳送到 {ip}:{listen_port} 失敗: {e}")

        packet_count += 1

        if time.time() - start_time >= 1.0:
            mb_sent = total_bytes_sent / (1024 * 1024)
            print(f"[Port {listen_port}] 封包數: {packet_count} | 傳送: {mb_sent:.2f} MB")
            packet_count = 0
            total_bytes_sent = 0
            start_time = time.time()

# ========= 多執行緒啟動每個 Port =========
for port in FORWARD_PORTS:
    t = threading.Thread(target=forward_packets, args=(port,), daemon=True)
    t.start()

# 主執行緒保持運行
try:
    while True:
        time.sleep(10)
except KeyboardInterrupt:
    print("\n[系統] 手動中斷，程式結束。")
