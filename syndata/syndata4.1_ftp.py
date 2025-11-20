import os
import time
import threading
import configparser
from datetime import datetime
from ftplib import FTP

BASE_DIR = os.getcwd()
CONFIG_FILE = os.path.join(BASE_DIR, "config.ini")

def get_ftp_config():
    """從 config.ini 讀取或建立 FTP 設定"""
    config = configparser.ConfigParser()

    # 若檔案存在就讀取
    if os.path.exists(CONFIG_FILE):
        config.read(CONFIG_FILE, encoding="utf-8")

    # 若無 FTP 區塊 → 建立
    if not config.has_section("FTP"):
        config.add_section("FTP")

    # 讀取若不存在就建立
    default_values = {
        "host": "192.168.138.207",
        "user": "admin",
        "password": "123",
        "folder": "/"
    }

    updated = False
    for key, val in default_values.items():
        if not config.has_option("FTP", key):
            config.set("FTP", key, val)
            updated = True

    # 寫回 config.ini
    if updated:
        with open(CONFIG_FILE, "w", encoding="utf-8") as f:
            config.write(f)

    # 回傳實際值
    return (
        config.get("FTP", "host").strip(),
        config.get("FTP", "user").strip(),
        config.get("FTP", "password").strip(),
        config.get("FTP", "folder").strip()
    )

# =====================================================
#                設定 FTP 連線資訊（改由 config.ini 讀取）
# =====================================================
ftp_host, ftp_user, ftp_pass, ftp_folder = get_ftp_config()
SCAN_INTERVAL = 5
disnamelen = 23
extranamelen = 89


# =====================================================
#                 讀取本地資料夾設定
# =====================================================
def get_sync_folder():
    config = configparser.ConfigParser()
    if os.path.exists(CONFIG_FILE):
        config.read(CONFIG_FILE, encoding="utf-8")

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



# =====================================================
#                 FTP 工具函式
# =====================================================
def ftp_makedirs(ftp, path):
    """確保 FTP Server 存在指定目錄，沒有就建立"""
    original = ftp.pwd()
    parts = path.strip("/").split("/")

    for p in parts:
        if not p:
            continue
        try:
            ftp.cwd(p)
        except:
            ftp.mkd(p)
            ftp.cwd(p)

    ftp.cwd(original)


def ftp_connect():
    """建立 FTP 連線（支援 UTF-8）"""
    ftp = FTP()
    ftp.encoding = "utf-8"    # <-- 重要：允許中文檔名
    ftp.connect(ftp_host, 21, timeout=10)
    ftp.login(ftp_user, ftp_pass)

    ftp_makedirs(ftp, ftp_folder)  # 確保資料夾存在
    ftp.cwd(ftp_folder)

    return ftp


def ftp_get_file_size(ftp: FTP, filename):
    """取得 FTP server 上某檔案大小，不存在則回傳 -1"""
    try:
        size = ftp.size(filename)
        return size
    except:
        return -1


# =====================================================
#                 使用 FTP 上傳檔案
# =====================================================
def encode_ftp_name(name):
    """FTP 指令只允許 Latin-1，因此需轉換以支援中文檔名"""
    return name.encode("utf-8").decode("latin-1")


def send_file_via_ftp(file_path):
    filename = os.path.basename(file_path)
    filesize = os.path.getsize(file_path)
    megabyte = 1024 * 1000

    try:
        ftp = ftp_connect()

        # 檢查是否存在相同檔案
        remote_filename = encode_ftp_name(filename)
        remote_size = ftp_get_file_size(ftp, remote_filename)
        if remote_size == filesize:
            print(f"[SKIP EXISTING] {filename} 已在 FTP 上且大小相同 → 跳過")
            ftp.quit()
            return True

        # 上傳開始
        print_name = (filename[:disnamelen - 3] + '...') if len(filename) > disnamelen else filename
        start_time = time.time()
        sent = 0

        with open(file_path, "rb") as f:

            def callback(chunk):
                nonlocal sent, start_time
                sent += len(chunk)
                elapsed = time.time() - start_time
                speed = sent / megabyte / elapsed if elapsed > 0 else 0
                eta = (filesize - sent) / megabyte / speed if speed > 0 else 0
                mm, ss = divmod(int(eta), 60)

                print(
                    f"\r[UPLOADING][FTP] ↑ {print_name}: "
                    f"{filesize / megabyte:.2f} MB / {sent / megabyte:.2f} MB "
                    f"({sent / filesize * 100:.2f}%, {speed:.2f} MB/s, {mm:02d}:{ss:02d})",
                    end="",
                    flush=True
                )

            ftp.storbinary(f"STOR {remote_filename}", f, blocksize=4096, callback=callback)

        ftp.quit()
        print("\n[SEND DONE][FTP] " + filename)
        return True

    except Exception as e:
        print(f"\n[SEND ERROR][FTP] {filename} - {e}")
        return False


# =====================================================
#                 主同步掃描函式
# =====================================================
def scan_and_sync_datasize_once():
    print("\n[SCAN] 掃描資料夾並同步...")

    for fname in os.listdir(FOLDER):
        full_path = os.path.join(FOLDER, fname)
        if os.path.isfile(full_path) and not fname.endswith(".tmp"):
            ok = send_file_via_ftp(full_path)
            if not ok:
                print(f"[RETRY NEXT ROUND] {fname}")

    print(f"[SYNCED][{datetime.now().strftime('%H:%M:%S')}] --- sync finished ---")



# =====================================================
#                     主程式 LOOP
# =====================================================
def main_loop():
    print(f"[INFO][{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] FTP 同步啟動")
    print(f"[INFO] 本地同步資料夾: {FOLDER}")

    while True:
        scan_and_sync_datasize_once()
        time.sleep(SCAN_INTERVAL)


if __name__ == "__main__":
    try:
        os.makedirs(FOLDER, exist_ok=True)
        main_loop()
    except KeyboardInterrupt:
        print("\n[EXIT] 使用者中斷")
    except Exception as e:
        print(f"[FATAL ERROR] {e}")
