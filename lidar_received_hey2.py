import os
import socket
import time
from datetime import datetime

SAVE_FOLDER = 'lidar_data'
PORT = 9000

def current_time():
    return datetime.now().strftime("[%Y-%m-%d %H:%M:%S]")

def handle_client(conn, addr):
    conn.settimeout(10)  # ⏱️ 最多等待 10 秒沒資料就斷開
    try:
        header = conn.recv(1024).decode()
        filename, filesize = header.split('|')
        filesize = int(filesize)

        conn.sendall(b'OK')
        print(f"{current_time()} 開始接收：{filename} ({filesize / 1024 / 1024:.2f} MB)")

        os.makedirs(SAVE_FOLDER, exist_ok=True)
        filepath = os.path.join(SAVE_FOLDER, filename)

        received = 0
        last_report = time.time()

        with open(filepath, 'wb') as f:
            while received < filesize:
                try:
                    chunk = conn.recv(min(4096, filesize - received))
                    if not chunk:
                        break
                except socket.timeout:
                    print(f"\n{current_time()} 連線逾時，停止接收 {filename}")
                    break

                f.write(chunk)
                received += len(chunk)

                if time.time() - last_report >= 1:
                    percent = (received / filesize) * 100
                    print(f"\r{current_time()} 已接收: {received / 1024 / 1024:.2f} / {filesize / 1024 / 1024:.2f} MB ({percent:.2f}%)", end='', flush=True)
                    last_report = time.time()
        
        print('\r' + ' ' * 100 + '\r', end='')  # 清空上一行
        if received == filesize:
            conn.sendall(b'DO')
            print(f"{current_time()} 接收完成: {filename}")
        else:
            print(f"{current_time()} 資料不完整: {filename} ({received / 1024 / 1024:.2f}/{filesize / 1024 / 1024:.2f} MB)")

    except Exception as e:
        print(f"{current_time()} 錯誤: {e}")
    finally:
        conn.close()

def main():
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        s.bind(('', PORT))
        s.listen(5)
        print(f"{current_time()} 接收伺服器啟動於 port {PORT}...")
        while True:
            conn, addr = s.accept()
            handle_client(conn, addr)

if __name__ == "__main__":
    main()
