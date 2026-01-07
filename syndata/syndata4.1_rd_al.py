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

def get_port():
    config = configparser.ConfigParser()
    if os.path.exists(CONFIG_FILE):
        config.read(CONFIG_FILE, encoding="utf-8")

    if not config.has_section("SETTINGS"):
        config.add_section("SETTINGS")

    if config.has_option("SETTINGS", "port"):
        try:
            port = int(config.get("SETTINGS", "port"))
            return port
        except ValueError:
            print("port 格式錯誤，使用預設值")

    port = 5001
    config.set("SETTINGS", "port", str(port))
    with open(CONFIG_FILE, "w", encoding="utf-8") as f:
        config.write(f)
    return port

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

    folder_name = "sync_data"
    full_path = os.path.join(BASE_DIR, folder_name)
    os.makedirs(full_path, exist_ok=True)

    config.set("SETTINGS", "sync_folder", folder_name)
    with open(CONFIG_FILE, "w") as f:
        config.write(f)

    return full_path

FOLDER = get_sync_folder()
PORT = get_port()
SCAN_INTERVAL = 5
file_status = {}
sending = 0
receiving = 0
disnamelen = 23
extranamelen = 89
recive_mod = 0

def get_radmin_ip():
    interfaces = psutil.net_if_addrs()
    for iface_name, iface_addrs in interfaces.items():
        if "Radmin" in iface_name:
            for addr in iface_addrs:
                if addr.family.name == 'AF_INET':
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

    iface, ip = get_radmin_ip()
    if ip:
        print(f"虛擬LAN介面：{iface}\n本機虛擬LAN IP位址：{ip}")
    else:
        print("未偵測到 Radmin 虛擬LAN介面")

    if os.path.exists(CONFIG_FILE):
        config.read(CONFIG_FILE)
        if "SETTINGS" in config and "receiver_ip" in config["SETTINGS"]:
            ip = config["SETTINGS"]["receiver_ip"].strip()
            if is_ip_reachable(ip):
                print(f"[SETTINGS] 使用儲存的 IP：{ip} 連線成功")
                return ip
            else:
                print(f"[CONNECTION ERROR] 無法連線到儲存的 IP：{ip}，請重新輸入。")

    timeoutT = 5
    print(f"[CONNECTING] 請輸入接收方的IP(輸入空白或等待{timeoutT}秒進入接收模式)")
    while True:
        ip_input = [None]

        def timed_input():
            ip_input[0] = input("IP：").strip()

        t = threading.Thread(target=timed_input)
        t.daemon = True
        t.start()
        t.join(timeoutT)

        ip = ip_input[0] or ""

        if not ip:
            print("")
            print("[RECEIVE MODE] 接收模式 ...")
            recive_mod = 1
            return None

        if is_ip_reachable(ip):
            print(f"[SYNC MODE] IP {ip} 連線成功，開始同步 ...")

            if not config.has_section("SETTINGS"):
                config.add_section("SETTINGS")

            config.set("SETTINGS", "receiver_ip", ip)
            with open(CONFIG_FILE, "w") as f:
                config.write(f)
            return ip
        else:
            timeoutT = 30
            print(f"[CONNECTION ERROR] 無法連線到 {ip}，請確認IP，輸入空白或等待{timeoutT}秒後自動進入接收模式。")

PEER_IP = get_peer_ip()

def send_file(file_path, relative_path):
    global sending, receiving
    filename = relative_path.replace("\\", "/")
    filesize = os.path.getsize(file_path)

    try:
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
            s.connect((PEER_IP, PORT))
            s.send(len(filename.encode()).to_bytes(4, 'big'))
            s.send(filename.encode())
            s.send(filesize.to_bytes(8, 'big'))

            sending = 1
            s.settimeout(10)
            try:
                response = s.recv(4).decode()
                if response == "SKIP":
                    print('\r' + ' ' * (disnamelen + extranamelen) + '\r', end='')
                    print(f"\r[SENDER][SKIP EXISTING] {filename}                                        ", end='', flush=True)
                    sending = 0
                    return True
            except socket.timeout:
                print('\r' + ' ' * (disnamelen + extranamelen) + '\r', end='')
                print(f"[SEND ERROR][{datetime.now().strftime('%H:%M:%S')}] {filename} - 等待對方回應超時 (10秒)")
                sending = 0
                return False
            finally:
                s.settimeout(None)

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
                    eta = (filesize - sent) / megabyte / speed if speed > 0 else 0
                    hh, mm, ss = time.gmtime(eta).tm_hour, time.gmtime(eta).tm_min, time.gmtime(eta).tm_sec
                    eta_str = f"{hh:d}:{mm:02d}:{ss:02d}" if hh > 0 else f"{mm:02d}:{ss:02d}"
                    print(f"\r[SENDING] ↑ 傳送中 {print_name}:{filesize / megabyte:.2f} MB/{sent / megabyte:.2f} MB({sent / filesize * 100:.2f}%, {speed:.2f} MB/S, {eta_str})", end='', flush=True)

            print('\r' + ' ' * (disnamelen + extranamelen) + '\r', end='')
        sending = 0
        print(f"[SEND DONE][{datetime.now().strftime('%H:%M:%S')}] {filename} ({filesize / megabyte:.2f} MB)")
        return True
    except Exception as e:
        sending = 0
        print('\r' + ' ' * (disnamelen + extranamelen) + '\r', end='')
        print(f"[SEND ERROR][{datetime.now().strftime('%H:%M:%S')}] {filename} - {e}")
        return False

def scan_and_sync_datasize_once():
    for root, dirs, files in os.walk(FOLDER):
        for fname in files:
            if fname.endswith(".tmp"):
                continue
            full_path = os.path.join(root, fname)
            rel_path = os.path.relpath(full_path, FOLDER)
            success = send_file(full_path, rel_path)
            if not success:
                print(f"[RETRY] 檔案 {rel_path} 同步失敗，將在下次掃描時再次嘗試")

    print('\r' + ' ' * (disnamelen + extranamelen) + '\r', end='')
    print(f"\r[SYNCED][{datetime.now().strftime('%H:%M:%S')}] ------- sync finished　----------", end='', flush=True)

def receiver():
    global sending, receiving
    os.makedirs(FOLDER, exist_ok=True)

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
        print(f"[RECEIVER][{datetime.now().strftime('%H:%M:%S')}] Listening on port {PORT}...")

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
                    os.makedirs(os.path.dirname(final_file_path), exist_ok=True)

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
                            hh, mm, ss = time.gmtime(eta).tm_hour, time.gmtime(eta).tm_min, time.gmtime(eta).tm_sec
                            if hh > 0:
                                eta_str = f"{hh:d}:{mm:02d}:{ss:02d}"
                            else:
                                eta_str = f"{mm:02d}:{ss:02d}"
                            print(
                                f"\r[RECEIVING][From:{addr[0]}] ↓ 接收中 "
                                f"{print_name}:{filesize / megabyte:.2f} MB/"
                                f"{received / megabyte:.2f} MB("
                                f"{received / filesize*100:.2f}%, "
                                f"{speed:.2f} MB/S, {eta_str})",
                                end='', flush=True
                            )


                    if os.path.exists(final_file_path):
                        os.remove(final_file_path)
                    os.rename(tmp_file_path, final_file_path)

                    print('\r' + ' ' * (disnamelen + extranamelen) + '\r', end='')
                    print(f"[RECEIVE DONE][{datetime.now().strftime('%H:%M:%S')}][From:{addr[0]}]{filename} ({filesize / (1024 * 1000):.2f} MB)")
                    receiving = 0

                except Exception as e:
                    receiving = 0
                    print('\r' + ' ' * (disnamelen + extranamelen) + '\r', end='')
                    print(f"[RECEIVE ERROR][{datetime.now().strftime('%H:%M:%S')}] - {e}")

def main_loop():
    print(f"[INFO][{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] 開始自動同步模式")
    threading.Thread(target=receiver, daemon=True).start()

    while True:
        if recive_mod == 1:
            time.sleep(1)
            continue

        if sending == 0 and receiving == 0:
            scan_and_sync_datasize_once()
        time.sleep(SCAN_INTERVAL)

if __name__ == "__main__":
    try:
        print(f"[INFO]目標資料夾: {FOLDER}")
        os.makedirs(FOLDER, exist_ok=True)
        main_loop()
    except KeyboardInterrupt:
        print("\n[EXIT] 使用者中斷，程式結束")
    except Exception as e:
        print(f"[FATAL ERROR] 未預期錯誤導致程式終止：{e}")
