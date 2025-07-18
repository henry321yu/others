import os
import time
import socket
import threading
import configparser
import subprocess
import platform
from datetime import datetime

BASE_DIR = os.path.dirname(os.path.abspath(__file__))
FOLDER = os.path.join(BASE_DIR, "sync_data")
CONFIG_FILE = os.path.join(BASE_DIR, "config.ini")
PORT = 5001
SCAN_INTERVAL = 5
file_status = {}
sending = 0

def is_ip_reachable(ip):
    count_flag = "-n" if platform.system().lower() == "windows" else "-c"
    try:
        result = subprocess.run(
            ["ping", count_flag, "1", ip],
            stdout=subprocess.DEVNULL,
            stderr=subprocess.DEVNULL
        )
        return result.returncode == 0
    except Exception:
        return False

def get_peer_ip():
    config = configparser.ConfigParser()
    if os.path.exists(CONFIG_FILE):
        config.read(CONFIG_FILE)
        if "SETTINGS" in config and "peer_ip" in config["SETTINGS"]:
            ip = config["SETTINGS"]["peer_ip"].strip()
            if is_ip_reachable(ip):
                print(f"[設定] 使用儲存的對方 IP：{ip} 連線成功")
                return ip
            else:
                print(f"[警告] 無法連線到儲存的 IP：{ip}，請重新輸入。")

    while True:
        ip = input("請輸入對方虛擬 IP：").strip()
        if not ip:
            print("[錯誤] IP 不可為空，請重新輸入。")
            continue
        if is_ip_reachable(ip):
            print(f"[成功] IP {ip} 連線成功")
            config["SETTINGS"] = {"peer_ip": ip}
            with open(CONFIG_FILE, "w") as f:
                config.write(f)
            return ip
        else:
            print(f"[失敗] 無法連線到 {ip}，請確認網路狀態後重試。")

PEER_IP = get_peer_ip()

def send_file(file_path):
    global sending
    filename = os.path.basename(file_path)
    filesize = os.path.getsize(file_path)
    try:
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
            s.connect((PEER_IP, PORT))
            s.send(len(filename.encode()).to_bytes(4, 'big'))
            s.send(filename.encode())
            s.send(filesize.to_bytes(8, 'big'))

            # 等待對方是否要跳過此檔案
            response = s.recv(4).decode()
            if response == "SKIP":
                print(f"[SKIP] {filename} 已存在於接收端")
                return
            sending = 1
            # print(f"\r[SEND START] {filename} ({filesize / (1024 * 1024):.2f} MB)", end='', flush=True)

            with open(file_path, 'rb') as f:
                sent = 0
                start_time = time.time()

                while chunk := f.read(4096):
                    s.sendall(chunk)
                    sent += len(chunk)
                    elapsed = time.time() - start_time
                    speed = sent / (1024 * 1024) / elapsed if elapsed > 0 else 0
                    print(f"\r[SEND START] → 傳送中 {filename}: {sent / (1024 * 1024):.2f} MB / {filesize / (1024 * 1024):.2f} MB ({speed:.2f} MB/s)", end='', flush=True)

        # 清除 SEND START 和 傳送進度輸出行
        print('\r' + ' ' * 200 + '\r', end='')
        print(f"[SEND DONE][{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] {filename} ({filesize / (1024 * 1024):.2f} MB)")
        sending = 0
    except Exception as e:
        print(f"[SEND ERROR][{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] {filename} - {e}")



def scan_and_sync():
    global file_status
    while True:
        current_status = {}
        for fname in os.listdir(FOLDER):
            full_path = os.path.join(FOLDER, fname)
            if os.path.isfile(full_path) and not fname.endswith(".tmp"):
                mtime = os.path.getmtime(full_path)
                current_status[fname] = mtime
                if fname not in file_status or mtime > file_status[fname]:
                    send_file(full_path)
        file_status = current_status
        time.sleep(SCAN_INTERVAL)

def receiver():
    global sending
    os.makedirs(FOLDER, exist_ok=True)
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        s.bind(('', PORT))
        s.listen(5)
        print(f"[RECEIVER][{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] Listening on port {PORT}...")

        while True:
            conn, addr = s.accept()
            with conn:
                while sending == 1:
                    time.sleep(1)
                try:
                    name_len = int.from_bytes(conn.recv(4), 'big')
                    filename = conn.recv(name_len).decode()
                    filesize = int.from_bytes(conn.recv(8), 'big')

                    final_file_path = os.path.join(FOLDER, filename)

                    # 檢查是否存在相同檔案（大小一致）
                    if os.path.exists(final_file_path) and os.path.getsize(final_file_path) == filesize:
                        conn.send(b"SKIP")
                        # print(f"[SKIP RECEIVED] {filename} 已存在，略過接收")
                        continue
                    else:
                        conn.send(b"OKAY")

                    tmp_file_path = final_file_path + ".tmp"
                    # print(f"\r[RECEIVE START] {filename} ({filesize / (1024 * 1024):.2f} MB)", end='', flush=True)

                    with open(tmp_file_path, 'wb') as f:
                        received = 0
                        start_time = time.time()
                        while received < filesize:
                            data = conn.recv(min(4096, filesize - received))
                            if not data:
                                break
                            f.write(data)
                            received += len(data)
                            elapsed = time.time() - start_time
                            speed = received / (1024 * 1024) / elapsed if elapsed > 0 else 0
                            print(f"\r[RECEIVE START] ← 接收中 {filename}: {received / (1024 * 1024):.2f} MB / {filesize / (1024 * 1024):.2f} MB ({speed:.2f} MB/s)", end='', flush=True)

                    os.rename(tmp_file_path, final_file_path)
                    print('\r' + ' ' * 200 + '\r', end='')   # 換行清除
                    print(f"[RECEIVE DONE][{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] {filename}")
                except Exception as e:
                    print(f"[RECEIVE ERROR][{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] - {e}")


if __name__ == "__main__":
    print(f"[START][{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] Synchronizing folder: {FOLDER}")
    os.makedirs(FOLDER, exist_ok=True)
    threading.Thread(target=scan_and_sync, daemon=True).start()
    receiver()
