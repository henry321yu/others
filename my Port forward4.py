import socket
import threading
import time

# ==========================================
# [1] 設定區塊 (可依需求自行增減)
# ==========================================

# 1. UDP 一對多廣播名單 (用於發送 LiDAR、影像、ADXL 等感測資料給遠端 PC)
REMOTE_PC_LIST = [
    # ('10.241.215.99', 0),  # dell
    # ('10.241.199.211', 0), # fly office
    ('26.55.45.86', 0),      # my new office (Radmin IP)
    # ('26.253.227.19', 0),  # my pc RADMIN
]

# 2. UDP 廣播監聽 Port (收到這些 Port 的封包，會群發給 REMOTE_PC_LIST)
UDP_BROADCAST_PORTS = [2370, 2385, 2386, 2368, 2369]

# 3. TCP 一對一轉發規則 (用於 SSH 或其他需建立連線的服務)
TCP_RULES = [
    # server
    {"local_port": 6969, "target_host": "10.241.20.154", "target_port": 6969},
    {"local_port": 6970, "target_host": "10.241.194.18", "target_port": 6969},

    # SSH
    {"local_port": 2222, "target_host": "10.241.194.18", "target_port": 22},
    {"local_port": 2223, "target_host": "10.241.20.154", "target_port": 22},
    {"local_port": 2224, "target_host": "10.241.156.153", "target_port": 22},
]

# 4. UDP 一對一代理規則 (用於控制訊號，例如從 Radmin 發送 Relay 指令回樹莓派)
UDP_PROXY_RULES = [
    {"local_port": 2371, "target_host": "10.241.156.153", "target_port": 2371},
]

# ==========================================
# [2] TCP 轉發邏輯 (一對一連線)
# ==========================================
def tcp_forward(source, destination):
    try:
        while True:
            data = source.recv(4096)
            if not data:
                break
            destination.sendall(data)
    except Exception:
        pass
    finally:
        source.close()
        destination.close()

def handle_tcp_client(client_socket, target_host, target_port):
    target_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    try:
        target_socket.connect((target_host, target_port))
        
        t1 = threading.Thread(target=tcp_forward, args=(client_socket, target_socket), daemon=True)
        t2 = threading.Thread(target=tcp_forward, args=(target_socket, client_socket), daemon=True)
        
        t1.start()
        t2.start()
        t1.join()
        t2.join()
    except Exception as e:
        print(f"[-] TCP 無法連線到 {target_host}:{target_port} -> {e}")
        client_socket.close()

def start_tcp_server(local_port, target_host, target_port):
    server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    try:
        server.bind(('0.0.0.0', local_port))
        server.listen(5)
        print(f"[*] TCP 監聽 Port {local_port} --> 轉發至 {target_host}:{target_port}")
        
        while True:
            client_socket, addr = server.accept()
            # print(f"[+] TCP 收到連線: {addr[0]}:{addr[1]} (目標: {target_host}:{target_port})")
            threading.Thread(target=handle_tcp_client, args=(client_socket, target_host, target_port), daemon=True).start()
    except Exception as e:
        print(f"[-] TCP 啟動失敗 (Port {local_port}): {e}")

# ==========================================
# [3] UDP 一對多廣播邏輯 (用於 LiDAR / 影像)
# ==========================================
def forward_udp_broadcast(listen_port):
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    sock.bind(('0.0.0.0', listen_port))
    
    # 計算本地檢視用的 Port (例如 2368 -> 12368)
    local_view_port = listen_port + 10000 
    print(f"[*] UDP 廣播監聽 Port {listen_port} --> 廣播至遠端，並開放本機 Port {local_view_port} 供讀取")

    packet_count = 0
    total_bytes_sent = 0
    start_time = time.time()

    while True:
        try:
            data, addr = sock.recvfrom(65536)
            # 防呆機制：過濾掉自己發給自己的封包，避免無限迴圈
            if addr[0] == '127.0.0.1':
                continue
        except Exception as e:
            continue

        # 1. 轉發給遠端 Radmin 電腦
        for ip, _ in REMOTE_PC_LIST:
            try:
                sent_bytes = sock.sendto(data, (ip, listen_port))
                total_bytes_sent += sent_bytes
            except Exception:
                pass
                
        # 2. 轉發給 IPC 本機 (加上偏移量，避免 Port 衝突)
        try:
            sock.sendto(data, ('127.0.0.1', local_view_port))
        except Exception:
            pass

        packet_count += 1

        # 每秒印出一次傳輸量統計
        if time.time() - start_time >= 1.0:
            if packet_count > 0:
                mb_sent = total_bytes_sent / (1024 * 1024)
                print(f"[Port {listen_port} 統計] 封包: {packet_count} | 流量: {mb_sent:.2f} MB/s")
            packet_count = 0
            total_bytes_sent = 0
            start_time = time.time()

# ==========================================
# [4] UDP 一對一代理邏輯 (用於 Relay 控制)
# ==========================================
def start_udp_proxy(local_port, target_host, target_port):
    local_server = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    local_server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    forward_client = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    
    try:
        local_server.bind(('0.0.0.0', local_port))
        print(f"[*] UDP 代理監聽 Port {local_port} --> 轉發至 {target_host}:{target_port}")
        
        last_client_addr = None
        while True:
            data, addr = local_server.recvfrom(65535)
            # 判斷是否為目標主機回傳的資料
            if addr[0] == target_host and addr[1] == target_port:
                if last_client_addr:
                    local_server.sendto(data, last_client_addr)
            else:
                last_client_addr = addr
                forward_client.sendto(data, (target_host, target_port))
    except Exception as e:
        print(f"[-] UDP 代理啟動失敗 (Port {local_port}): {e}")

# ==========================================
# [5] 主程式 (啟動所有執行緒)
# ==========================================
def main():
    print("========================================")
    print("      綜合通訊轉發伺服器 (支援 Radmin)      ")
    print("========================================\n")
    
    threads = []
    
    # 啟動 TCP 規則
    for rule in TCP_RULES:
        t = threading.Thread(target=start_tcp_server, args=(rule["local_port"], rule["target_host"], rule["target_port"]))
        t.daemon = True
        t.start()
        threads.append(t)
        
    # 啟動 UDP 廣播規則 (LiDAR 等)
    for port in UDP_BROADCAST_PORTS:
        t = threading.Thread(target=forward_udp_broadcast, args=(port,))
        t.daemon = True
        t.start()
        threads.append(t)
        
    # 啟動 UDP 代理規則 (Relay 等)
    for rule in UDP_PROXY_RULES:
        t = threading.Thread(target=start_udp_proxy, args=(rule["local_port"], rule["target_host"], rule["target_port"]))
        t.daemon = True
        t.start()
        threads.append(t)
        
    print("\n[系統] 所有服務啟動完成。按 Ctrl+C 可中斷。\n")
    
    try:
        while True:
            time.sleep(10)
    except KeyboardInterrupt:
        print("\n[系統] 偵測到中斷訊號，程式結束。")

if __name__ == '__main__':
    main()