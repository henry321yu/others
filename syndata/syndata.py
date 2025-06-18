import os
import time
import socket
import threading
from datetime import datetime

# === 設定 ===
FOLDER = os.path.join(os.path.dirname(os.path.abspath(__file__)), "sync_data")
PEER_IP = '10.241.183.64'  # ⚠️← 每台電腦要改成「對方」的虛擬 IP
PORT = 5001
SCAN_INTERVAL = 5  # 每幾秒掃描一次

# === 儲存檔案狀態 ===
file_status = {}

def send_file(file_path):
    filename = os.path.basename(file_path)
    filesize = os.path.getsize(file_path)

    try:
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
            s.connect((PEER_IP, PORT))
            s.send(len(filename.encode()).to_bytes(4, 'big'))
            s.send(filename.encode())
            s.send(filesize.to_bytes(8, 'big'))

            with open(file_path, 'rb') as f:
                while chunk := f.read(4096):
                    s.sendall(chunk)
            print(f"[SEND][{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] {filename}")
    except Exception as e:
        print(f"[SEND ERROR][{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] {filename} - {e}")

def scan_and_sync():
    global file_status
    while True:
        current_status = {}
        for fname in os.listdir(FOLDER):
            full_path = os.path.join(FOLDER, fname)
            if os.path.isfile(full_path):
                mtime = os.path.getmtime(full_path)
                current_status[fname] = mtime
                if fname not in file_status or mtime > file_status[fname]:
                    send_file(full_path)
        file_status = current_status
        time.sleep(SCAN_INTERVAL)

def receiver():
    os.makedirs(FOLDER, exist_ok=True)
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        s.bind(('', PORT))
        s.listen(5)
        print(f"[RECEIVER][{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] Listening on port {PORT}...")

        while True:
            conn, addr = s.accept()
            with conn:
                try:
                    name_len = int.from_bytes(conn.recv(4), 'big')
                    filename = conn.recv(name_len).decode()
                    filesize = int.from_bytes(conn.recv(8), 'big')
                    file_path = os.path.join(FOLDER, filename)

                    with open(file_path, 'wb') as f:
                        received = 0
                        while received < filesize:
                            data = conn.recv(min(4096, filesize - received))
                            if not data:
                                break
                            f.write(data)
                            received += len(data)

                    file_status[filename] = os.path.getmtime(file_path)
                    print(f"[RECEIVED][{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] {filename}")
                except Exception as e:
                    print(f"[RECEIVE ERROR][{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] - {e}")

if __name__ == "__main__":
    print(f"[START][{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] Synchronizing folder: {FOLDER}")
    threading.Thread(target=scan_and_sync, daemon=True).start()
    receiver()