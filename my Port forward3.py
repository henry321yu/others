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
        # 連線到樹莓派
        target_socket.connect((target_host, target_port))
        
        # 建立雙向轉發執行緒
        client_to_target = threading.Thread(target=forward, args=(client_socket, target_socket))
        target_to_client = threading.Thread(target=forward, args=(target_socket, client_socket))
        
        client_to_target.start()
        target_to_client.start()
        
        client_to_target.join()
        target_to_client.join()
    except Exception as e:
        print(f"[-] 無法連線到目標伺服器: {e}")
        client_socket.close()

def main():
    # 辦公室電腦的設定
    LOCAL_HOST = '0.0.0.0'  # 0.0.0.0 代表監聽所有網路介面 (包含 Radmin VPN)
    LOCAL_PORT = 6970       # Radmin VPN 電腦要連線的 Port

    # 樹莓派的設定
    TARGET_HOST = '10.241.194.18' # 請替換成樹莓派的真實 IP
    TARGET_PORT = 6969         # 樹莓派的 Port

    server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server.bind((LOCAL_HOST, LOCAL_PORT))
    server.listen(5)
    
    print(f"[*] 正在監聽本機 {LOCAL_HOST}:{LOCAL_PORT} ...")
    print(f"[*] 準備轉發至樹莓派 {TARGET_HOST}:{TARGET_PORT} ...")

    while True:
        client_socket, addr = server.accept()
        print(f"[+] 收到來自 {addr[0]}:{addr[1]} 的連線")
        # 每一個新連線開一個獨立執行緒處理
        client_thread = threading.Thread(target=handle_client, args=(client_socket, TARGET_HOST, TARGET_PORT))
        client_thread.start()

if __name__ == '__main__':
    main()