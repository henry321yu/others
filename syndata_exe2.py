import os
import time
import socket
import threading
import configparser
import subprocess
import platform
from datetime import datetime
import psutil

BASE_DIR = os.getcwd()
FOLDER = os.path.join(BASE_DIR, "sync_data")
CONFIG_FILE = os.path.join(BASE_DIR, "config.ini")
PORT = 5001
SCAN_INTERVAL = 1
file_status = {}
sending = 0

def get_zerotier_ip():
    interfaces = psutil.net_if_addrs()
    for iface_name, iface_addrs in interfaces.items():
        if "Zero" in iface_name:  # ZeroTier 介面通常以 zt 開頭
            for addr in iface_addrs:
                if addr.family.name == 'AF_INET':  # IPv4
                    return iface_name, addr.address
    return None, None

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

    iface, ip = get_zerotier_ip()
    if ip:
        print(f"ZeroTier介面：{iface}\n本機IP位址：{ip}")
    else:
        print("找不到本機ZeroTier IP")

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
                    print_name = (filename[:37] + '...') if len(filename) > 30 else filename
                    print(f"\r[SEND START] ↑ 傳送中 {print_name}: {filesize / (1024 * 1024):.2f} MB/{sent / (1024 * 1024):.2f} MB({sent / filesize*100:.2f}% , {speed:.2f} MB/S)", end='', flush=True)

        # 清除 SEND START 和 傳送進度輸出行
        print('\r' + ' ' * 40 + '\r', end='')
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
    # 啟動時清理 .tmp 檔
    for file in os.listdir(FOLDER):
        if file.endswith(".tmp"):
            tmp_path = os.path.join(FOLDER, file)
            try:
                os.remove(tmp_path)
                print(f"[CLEANUP] 開始前已刪除殘留暫存檔: {tmp_path}")
            except Exception as e:
                print(f"[CLEANUP ERROR] 無法刪除 {tmp_path}: {e}")
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
                            print_name = (filename[:37] + '...') if len(filename) > 40 else filename
                            print(f"\r[RECEIVE START] ↓ 接收中 {print_name}: {filesize / (1024 * 1024):.2f} MB/{received / (1024 * 1024):.2f} MB({received / filesize*100:.2f}% , {speed:.2f} MB/S)", end='', flush=True)

                    os.rename(tmp_file_path, final_file_path)
                    print('\r' + ' ' * 40 + '\r', end='')   # 清除行
                    print(f"[RECEIVE DONE][{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] {filename}")
                except Exception as e:
                    print(f"[RECEIVE ERROR][{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] - {e}")
                    if tmp_file_path and os.path.exists(tmp_file_path):
                        try:
                            os.remove(tmp_file_path)
                            print(f"[CLEANUP] 已刪除不完整檔案 {tmp_file_path}")
                        except Exception as cleanup_error:
                            print(f"[CLEANUP ERROR] 刪除失敗: {cleanup_error}")

if __name__ == "__main__":
    print(f"[START][{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] Synchronizing folder: {FOLDER}")
    os.makedirs(FOLDER, exist_ok=True)
    threading.Thread(target=scan_and_sync, daemon=True).start()
    receiver()