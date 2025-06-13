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
CONFIG_FILE = os.path.join(BASE_DIR, "config.ini")
def get_sync_folder():
    config = configparser.ConfigParser()
    if os.path.exists(CONFIG_FILE):
        config.read(CONFIG_FILE)
    
    if not config.has_section("SETTINGS"):
        config.add_section("SETTINGS")
    
    if config.has_option("SETTINGS", "sync_folder"):
        folder_name = config.get("SETTINGS", "sync_folder").strip()
        if folder_name:
            return os.path.join(BASE_DIR, folder_name)
    
    # 預設為 sync_data
    folder_name = "sync_data"
    full_path = os.path.join(BASE_DIR, folder_name)
    os.makedirs(full_path, exist_ok=True)
    
    config.set("SETTINGS", "sync_folder", folder_name)
    with open(CONFIG_FILE, "w") as f:
        config.write(f)
    
    return full_path
FOLDER = get_sync_folder()
PORT = 5001
SCAN_INTERVAL = 3
file_status = {}
sending = 0
receiving = 0
disnamelen = 30
extranamelen = 87
recive_mod = 0

def get_zerotier_ip():
    interfaces = psutil.net_if_addrs()
    for iface_name, iface_addrs in interfaces.items():
        if "Zero" in iface_name:  # 介面通常開頭
            for addr in iface_addrs:
                if addr.family.name == 'AF_INET':  # IPv4
                    return iface_name, addr.address
    return None, None

def get_radmin_ip():
    interfaces = psutil.net_if_addrs()
    for iface_name, iface_addrs in interfaces.items():
        if "Radmin" in iface_name:  # 介面通常開頭
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
    global recive_mod
    config = configparser.ConfigParser()

    iface, ip = get_zerotier_ip()
    # iface, ip = get_radmin_ip()
    if ip:
        print(f"虛擬LAN介面：{iface}\n本機虛擬LAN IP位址：{ip}")
    else:
        print("未偵測到 ZeroTier 虛擬LAN介面")
        # print("未偵測到 Radmin 虛擬LAN介面")

    if os.path.exists(CONFIG_FILE):
        config.read(CONFIG_FILE)
        if "SETTINGS" in config and "receiver_ip" in config["SETTINGS"]:
            ip = config["SETTINGS"]["receiver_ip"].strip()
            if is_ip_reachable(ip):
                print(f"[SETTINGS] 使用儲存的 IP：{ip} 連線成功")
                return ip
            else:
                print(f"[CONNECTION ERROR] 無法連線到儲存的 IP：{ip}，請重新輸入。")

    while True:
        ip = input("請輸入接收方的IP(空白或等待10秒則為純接收模式)，連線成功後會存至config.ini：").strip()
        if not ip:
            print("[INFO] 純接收模式啟用（未指定對方 IP）")
            recive_mod = 1
            return None  # 代表接收模式
        if is_ip_reachable(ip):
            print(f"[SUCCESSED] IP {ip} 連線成功")
            config["SETTINGS"] = {"receiver_ip": ip}
            with open(CONFIG_FILE, "w") as f:
                config.write(f)
            return ip
        else:
            print(f"[CONNECTION ERROR] 無法連線到 {ip}，請確認網路狀態後重試。")

PEER_IP = get_peer_ip()

def send_file(file_path):
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
                print(f"[SKIP EXISTING] {filename}")
                return True
            sending = 1

            with open(file_path, 'rb') as f:
                sent = 0
                start_time = time.time()

                while chunk := f.read(4096):
                    s.sendall(chunk)
                    sent += len(chunk)
                    elapsed = time.time() - start_time                    
                    megabyte = 1024 * 1000
                    speed = sent / megabyte / elapsed if elapsed > 0 else 0
                    print_name = (filename[:disnamelen - 3] + '...') if len(filename) > disnamelen else filename            
                    eta = (filesize - sent)/ megabyte / speed if speed > 0 else 0
                    eta_str = time.strftime('%M:%S', time.gmtime(eta))
                    print(f"\r[SEND START] ↑ 傳送中 {print_name}:{filesize / megabyte:.2f} MB/{sent / megabyte:.2f} MB({sent / filesize*100:.2f}% , {speed:.2f} MB/S, ETA: {eta_str})", end='', flush=True)

                # 清除 SEND START 和 傳送進度輸出行
                print('\r' + ' ' * (disnamelen + extranamelen) + '\r', end='')
        print(f"[SEND DONE][{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] {filename} ({filesize / megabyte:.2f} MB)")
        sending = 0
        return True
    except Exception as e:        
        print('\r' + ' ' * (disnamelen + extranamelen) + '\r', end='')
        print(f"[SEND ERROR][{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] {filename} - {e}")
        return False 

def scan_and_sync_datasize():
    global sending , receiving
    while True:    
        while receiving == 1:
            time.sleep(1)
        print("[INFO] ------- new scan and sync -------")
        for fname in os.listdir(FOLDER):
            full_path = os.path.join(FOLDER, fname)
            if os.path.isfile(full_path) and not fname.endswith(".tmp"):
                fsize = os.path.getsize(full_path)
                success = send_file(full_path)
                if success:
                    continue
                else:
                    print(f"[RETRY] 檔案 {fname} 傳送失敗，將在下次掃描時再次嘗試")
        time.sleep(SCAN_INTERVAL)

def receiver():
    global sending , receiving
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
                        continue
                    else:
                        conn.send(b"OKAY")

                    receiving = 1

                    tmp_file_path = final_file_path + ".tmp"
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
                            megabyte = 1024 * 1000
                            speed = received / megabyte / elapsed if elapsed > 0 else 0
                            print_name = (filename[:disnamelen-3] + '...') if len(filename) > disnamelen else filename
                            eta = (filesize - received)/ megabyte / speed if speed > 0 else 0
                            eta_str = time.strftime('%M:%S', time.gmtime(eta))
                            print(f"\r[RECEIVE START] ↓ 接收中 {print_name}:{filesize / megabyte:.2f} MB/{received / megabyte:.2f} MB({received / filesize*100:.2f}% , {speed:.2f} MB/S, ETA: {eta_str})", end='', flush=True)

                    if os.path.exists(final_file_path):
                        os.remove(final_file_path)  # 刪除已存在的同名檔案
                    os.rename(tmp_file_path, final_file_path)
                    print('\r' + ' ' * (disnamelen + extranamelen) + '\r', end='')   # 清除行
                    print(f"[RECEIVE DONE][{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] {filename} ({filesize / megabyte:.2f} MB)")
                    receiving = 0
                except Exception as e:
                    print('\r' + ' ' * (disnamelen + extranamelen) + '\r', end='')   # 清除行
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
    if recive_mod == 0:
        threading.Thread(target=scan_and_sync_datasize, daemon=True).start() #依據檔案大小更新
    receiver()