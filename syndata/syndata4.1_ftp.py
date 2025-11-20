import os
import time
import threading
import configparser
from datetime import datetime
from ftplib import FTP

BASE_DIR = os.getcwd()
CONFIG_FILE = os.path.join(BASE_DIR, "config.ini")

# =====================================================
#                設定 FTP 連線資訊
# =====================================================
ftp_host = "192.168.138.207"   # ← 修改成你的 FTP Server IP
ftp_user = "admin"      # ← 修改帳號
ftp_pass = "123"      # ← 修改密碼
ftp_folder = "/"            # ← FTP Server 目錄，可改
SCAN_INTERVAL = 5
disnamelen = 23
extranamelen = 89


# =====================================================
#                 讀取本地資料夾設定
# =====================================================
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



# =====================================================
#                 FTP 工具函式
# =====================================================
def ftp_connect():
    """建立 FTP 連線"""
    ftp = FTP()
    ftp.connect(ftp_host, 21, timeout=10)
    ftp.login(ftp_user, ftp_pass)
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
def send_file_via_ftp(file_path):
    filename = os.path.basename(file_path)
    filesize = os.path.getsize(file_path)
    megabyte = 1024 * 1000

    try:
        ftp = ftp_connect()

        # 檢查是否存在且大小相同
        remote_size = ftp_get_file_size(ftp, filename)
        if remote_size == filesize:
            print(f"[SKIP EXISTING] {filename} 已在 FTP 上且大小相同 → 跳過")
            ftp.quit()
            return True

        # 開始上傳
        print_name = (filename[:disnamelen - 3] + '...') if len(filename) > disnamelen else filename
        start_time = time.time()

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
                    f"{filesize/megabyte:.2f} MB / {sent/megabyte:.2f} MB "
                    f"({sent/filesize*100:.2f}%, {speed:.2f} MB/s, {mm:02d}:{ss:02d})",
                    end="",
                    flush=True
                )

            sent = 0
            ftp.storbinary(f"STOR {filename}", f, blocksize=4096, callback=callback)

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
