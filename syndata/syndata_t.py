# file_sync_sender.py（電腦A執行）
import os
import time
import socket
from datetime import datetime

FOLDER = os.path.expanduser("~/Desktop/sync_data")
DEST_IP = '10.241.183.64'  # ← 請替換成 B 的虛擬 IP
PORT = 5001
SCAN_INTERVAL = 5  # 每 5 秒掃描一次

# 儲存檔案狀態（路徑 → 最後修改時間）
file_status = {}

def send_file(file_path):
    filename = os.path.basename(file_path)
    filesize = os.path.getsize(file_path)

    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        try:
            s.connect((DEST_IP, PORT))
        except Exception as e:
            print(f"[Sender][{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] Cannot connect to {DEST_IP}:{PORT} - {e}")
            return

        # 傳送檔名長度、檔名、檔案大小
        s.send(len(filename.encode()).to_bytes(4, 'big'))
        s.send(filename.encode())
        s.send(filesize.to_bytes(8, 'big'))

        with open(file_path, 'rb') as f:
            while chunk := f.read(4096):
                s.sendall(chunk)
        print(f"[Sender] [{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] Sent: {filename}")

def scan_and_sync():
    global file_status
    current_status = {}
    for fname in os.listdir(FOLDER):
        full_path = os.path.join(FOLDER, fname)
        if os.path.isfile(full_path):
            mtime = os.path.getmtime(full_path)
            current_status[fname] = mtime
            # 如果檔案是新的或被修改過
            if fname not in file_status or mtime > file_status[fname]:
                send_file(full_path)

    file_status = current_status

if __name__ == "__main__":
    print(f"[Sender][{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] Watching folder: {FOLDER}")
    while True:
        scan_and_sync()
        time.sleep(SCAN_INTERVAL)
