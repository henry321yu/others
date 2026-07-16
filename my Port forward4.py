import socket
import threading
import time

# ==========================================
# TCP 轉發邏輯 (用於 SSH 等需要建立連線的服務)
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
        
        client_to_target = threading.Thread(target=tcp_forward, args=(client_socket, target_socket), daemon=True)
        target_to_client = threading.Thread(target=tcp_forward, args=(target_socket, client_socket), daemon=True)
        
        client_to_target.start()
        target_to_client.start()
        
        client_to_target.join()
        target_to_client.join()
    except Exception as e:
        print(f"[-] TCP 無法連線到目標 {target_host}:{target_port} -> {e}")
        client_socket.close()

def start_tcp_server(local_port, target_host, target_port):
    server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    try:
        server.bind(('0.0.0.0', local_port))
        server.listen(5)
        print(f"[*] TCP 成功啟動：Port {local_port} --> {target_host}:{target_port}")
        while True:
            client_socket, addr = server.accept()
            threading.Thread(target=handle_tcp_client, args=(client_socket, target_host, target_port), daemon=True).start()
    except Exception as e:
        print(f"[-] TCP 啟動失敗 (Port {local_port}): {e}")

# ==========================================
# UDP 轉發邏輯 (用於 LiDAR, 影像, ADXL 等感測器資料)
# ==========================================
def start_udp_server(local_port, target_host, target_port):
    # 用來接收來源資料的 Socket
    local_server = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    # 用來把資料轉發給目標的 Socket
    forward_client = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    
    try:
        local_server.bind(('0.0.0.0', local_port))
        print(f"[*] UDP 成功啟動：Port {local_port} --> {target_host}:{target_port}")
        
        # 紀錄最後發送者的位置 (若有雙向需求，如 Relay 控制)
        last_client_addr = None
        
        while True:
            # UDP 最大封包大小通常設為 65535
            data, addr = local_server.recvfrom(65535)
            
            # 如果資料來自目標主機 (例如對方回傳 Relay 指令)
            if addr[0] == target_host and addr[1] == target_port:
                if last_client_addr:
                    local_server.sendto(data, last_client_addr)
            # 如果資料來自樹莓派或其他來源
            else:
                last_client_addr = addr
                forward_client.sendto(data, (target_host, target_port))
                
    except Exception as e:
        print(f"[-] UDP 啟動失敗 (Port {local_port}): {e}")

# ==========================================
# 主程式：啟動所有規則
# ==========================================
def main():
    # 在規則中加入 "protocol" 來區分 TCP 或 UDP
    forwarding_rules = [
    
        {"protocol": "TCP", "local_port": 6969, "target_host": "10.241.20.154", "target_port": 6969},
        {"protocol": "TCP", "local_port": 6970, "target_host": "10.241.194.18", "target_port": 6969},

        # SSH 使用 TCP
        {"protocol": "TCP", "local_port": 2222, "target_host": "10.241.194.18", "target_port": 22},
        {"protocol": "TCP", "local_port": 2223, "target_host": "10.241.20.154", "target_port": 22},
        {"protocol": "TCP", "local_port": 2224, "target_host": "10.241.156.153", "target_port": 22},

        # 感測器資料使用 UDP
        {"protocol": "UDP", "local_port": 2370, "target_host": "10.241.156.153", "target_port": 2370},
        {"protocol": "UDP", "local_port": 2385, "target_host": "10.241.156.153", "target_port": 2385},
        {"protocol": "UDP", "local_port": 2386, "target_host": "10.241.156.153", "target_port": 2386},
        {"protocol": "UDP", "local_port": 2368, "target_host": "10.241.156.153", "target_port": 2368},
        {"protocol": "UDP", "local_port": 2369, "target_host": "10.241.156.153", "target_port": 2369},
        
        # Relay 控制也是 UDP
        {"protocol": "UDP", "local_port": 2371, "target_host": "10.241.156.153", "target_port": 2371},
    ]
    
    print("[*] 正在啟動多重 Port Forwarding 服務...\n")
    server_threads = []
    
    for rule in forwarding_rules:
        if rule["protocol"].upper() == "TCP":
            target = start_tcp_server
        else:
            target = start_udp_server
            
        t = threading.Thread(
            target=target,
            args=(rule["local_port"], rule["target_host"], rule["target_port"])
        )
        t.daemon = True
        t.start()
        server_threads.append(t)
        
    try:
        while True:
            time.sleep(1)
    except KeyboardInterrupt:
        print("\n[*] 偵測到中斷訊號，正在關閉服務...")

if __name__ == '__main__':
    main()