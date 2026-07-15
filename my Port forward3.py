import socket
import threading

# 轉發資料的核心函式
def forward(source, destination):
    try:
        while True:
            data = source.recv(4096)
            if not data:
                break
            destination.sendall(data)
    except Exception as e:
        pass
    finally:
        source.close()
        destination.close()

# 處理新連線
def handle_client(client_socket, target_host, target_port):
    target_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    try:
        # 連線到目標伺服器 (樹莓派)
        target_socket.connect((target_host, target_port))
        
        # 建立雙向轉發執行緒
        client_to_target = threading.Thread(target=forward, args=(client_socket, target_socket))
        target_to_client = threading.Thread(target=forward, args=(target_socket, client_socket))
        
        # 設定為 daemon，讓主程式結束時能自動關閉這些執行緒
        client_to_target.daemon = True
        target_to_client.daemon = True
        
        client_to_target.start()
        target_to_client.start()
        
        client_to_target.join()
        target_to_client.join()
    except Exception as e:
        print(f"[-] 無法連線到目標伺服器 {target_host}:{target_port} -> {e}")
        client_socket.close()

# 獨立的監聽伺服器函式 (每組規則都會跑一個這個)
def start_server(local_port, target_host, target_port):
    local_host = '0.0.0.0'
    server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    
    # 加入這行可避免重開程式時發生 "Address already in use" 錯誤
    server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    
    try:
        server.bind((local_host, local_port))
        server.listen(5)
        print(f"[*] 成功啟動：本機 Port {local_port} --> 轉發至 {target_host}:{target_port}")
        
        while True:
            client_socket, addr = server.accept()
            print(f"[+] 收到連線: 來自 {addr[0]}:{addr[1]} (目標: {target_host}:{target_port})")
            
            # 每一個新連線開一個獨立執行緒處理
            client_thread = threading.Thread(
                target=handle_client, 
                args=(client_socket, target_host, target_port)
            )
            client_thread.daemon = True
            client_thread.start()
            
    except Exception as e:
        print(f"[-] 啟動監聽失敗 (Port {local_port}): {e}")

def main():
    # ==========================================
    # 在這裡設定所有的轉發規則 (可自由新增)
    # ==========================================
    forwarding_rules = [
        # 規則 1: 原本的樹莓派
        {"local_port": 6970, "target_host": "10.241.194.18", "target_port": 6969},
        
        # 規則 2: 另一台設備或另一個 Port (範例)
        # {"local_port": 8080, "target_host": "10.241.194.19", "target_port": 80},
        
        # 規則 3: 同一台樹莓派的不同 Port (範例：SSH)
        # {"local_port": 2222, "target_host": "10.241.194.18", "target_port": 22},
    ]

    print("[*] 正在啟動多重 Port Forwarding 服務...\n")
    
    # 存放伺服器執行緒的清單
    server_threads = []
    
    # 為每一組規則啟動一個獨立的監聽執行緒
    for rule in forwarding_rules:
        t = threading.Thread(
            target=start_server,
            args=(rule["local_port"], rule["target_host"], rule["target_port"])
        )
        t.daemon = True
        t.start()
        server_threads.append(t)
        
    # 讓主執行緒保持運行，並捕捉 Ctrl+C 來安全退出
    try:
        for t in server_threads:
            # 使用迴圈卡住主執行緒，避免程式直接結束
            while t.is_alive():
                t.join(1)
    except KeyboardInterrupt:
        print("\n[*] 偵測到中斷訊號，正在關閉服務...")

if __name__ == '__main__':
    main()