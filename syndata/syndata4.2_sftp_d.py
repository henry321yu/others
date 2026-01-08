import os
import time
import configparser
import paramiko
from datetime import datetime
from stat import S_ISDIR

# =====================================================
#                     基本設定
# =====================================================
BASE_DIR = os.path.dirname(os.path.abspath(__file__))
CONFIG_FILE = os.path.join(BASE_DIR, "config_sftp_d.ini")
disnamelen = 23
extranamelen = 89
SCAN_INTERVAL = 5

# =====================================================
#                   讀取 config
# =====================================================
def get_config():
    cfg = configparser.ConfigParser()

    default_cfg = {
        "SFTP": {
            "host": "127.0.0.1",
            "port": "22",
            "user": "",
            "password": "",
            "target_folder": "/",
            "sync_subdirs": "yes" 
        },
        "LOCAL": {
            "local_folder": "sftp_download"
        }
    }

    if not os.path.exists(CONFIG_FILE):
        cfg.read_dict(default_cfg)
        with open(CONFIG_FILE, "w", encoding="utf-8") as f:
            cfg.write(f)
        print("[CONFIG] 已建立預設設定檔")
        print(f"[CONFIG] 請編輯設定檔後儲存：{CONFIG_FILE}")

        last_mtime = os.path.getmtime(CONFIG_FILE)

        while True:
            time.sleep(3)
            print(f"[WAIT] 等待使用者編輯設定檔：{CONFIG_FILE}")

            try:
                current_mtime = os.path.getmtime(CONFIG_FILE)
            except FileNotFoundError:
                continue

            if current_mtime != last_mtime:
                print("[CONFIG] 偵測到設定檔變更，重新讀取")
                cfg.read(CONFIG_FILE, encoding="utf-8")
                break
    else:
        cfg.read(CONFIG_FILE, encoding="utf-8")

    sync_subdirs = cfg.get("SFTP", "sync_subdirs", fallback="yes").lower() == "yes"

    return (
        cfg.get("SFTP", "host"),
        cfg.getint("SFTP", "port"),
        cfg.get("SFTP", "user"),
        cfg.get("SFTP", "password"),
        cfg.get("SFTP", "target_folder"),
        os.path.join(BASE_DIR, cfg.get("LOCAL", "local_folder")),
        sync_subdirs
    )

# =====================================================
#             SFTP 遞迴下載主函式
# =====================================================
def format_size(size_bytes):
    """自動將大小轉換為 B/KB/MB/GB"""
    if size_bytes < 1024:
        return f"{size_bytes:.2f} bytes"
    elif size_bytes < 1024**2:
        return f"{size_bytes/1024:.2f} KB"
    elif size_bytes < 1024**3:
        return f"{size_bytes/1024**2:.2f} MB"
    else:
        return f"{size_bytes/1024**3:.2f} GB"

def sftp_download(sftp, remote_dir, local_dir, sync_subdirs):
    os.makedirs(local_dir, exist_ok=True)

    try:
        items = sftp.listdir_attr(remote_dir)
    except Exception as e:
        print(f"[ERROR] 無法存取 SFTP 目錄：{remote_dir} - {e}")
        return

    for attr in items:
        r_path = f"{remote_dir}/{attr.filename}".replace("//", "/")
        l_path = os.path.join(local_dir, attr.filename)

        # ---------- 目錄 ----------
        if S_ISDIR(attr.st_mode):
            if sync_subdirs:
                sftp_download(sftp, r_path, l_path, sync_subdirs)
            else:
                print(f"[SKIP DIR] {r_path}")
            continue

        remote_size = attr.st_size
        if os.path.exists(l_path) and os.path.getsize(l_path) == remote_size:
            print(f"[SKIP EXISTING] {l_path}")
            continue

        print_name = (
            attr.filename[:disnamelen-3] + "..."
            if len(attr.filename) > disnamelen else attr.filename
        )

        start_time = time.time()
        downloaded = 0
        last_len = 0

        def progress(transferred, total):
            nonlocal downloaded, last_len
            downloaded = transferred
            elapsed = time.time() - start_time
            speed = downloaded / elapsed if elapsed > 0 else 0  # bytes/s
            percent = downloaded / total * 100 if total > 0 else 0
            eta = (total - downloaded) / speed if speed > 0 else 0
            hh, mm, ss = int(eta // 3600), int((eta % 3600) // 60), int(eta % 60)
            eta_str = f"{hh:d}:{mm:02d}:{ss:02d}" if hh > 0 else f"{mm:02d}:{ss:02d}"

            progress_line = (
                f"[DOWNLOADING][SFTP] ↓ {print_name}: "
                f"{format_size(downloaded)}/{format_size(total)} "
                f"({percent:.2f}%, {format_size(speed)}/s, {eta_str})"
            )

            # 計算剩餘空格覆蓋舊行
            extra_space = max(last_len - len(progress_line), 0)
            print('\r' + progress_line + ' ' * extra_space, end='', flush=True)
            last_len = len(progress_line)

        tmp_path = l_path + ".tmp"
        try:
            sftp.get(r_path, tmp_path, callback=progress)
            os.replace(tmp_path, l_path)

            # 清除進度行
            print('\r' + ' ' * (disnamelen + extranamelen) + '\r', end='')

            print(f"[DOWNLOAD DONE][{datetime.now().strftime('%H:%M:%S')}] "
                  f"{l_path} ({format_size(remote_size)})")

        except Exception as e:
            if os.path.exists(tmp_path):
                os.remove(tmp_path)
            # 清除進度行
            print('\r' + ' ' * (disnamelen + extranamelen) + '\r', end='')
            print(f"[DOWNLOAD ERROR][{datetime.now().strftime('%H:%M:%S')}] {r_path} - {e}")
            continue

# =====================================================
#               SFTP 連線 / 重連
# =====================================================
def connect_sftp(host, port, user, password):
    while True:
        try:
            print(f"[CONNECT] SFTP {host}:{port}")
            transport = paramiko.Transport((host, port))
            transport.connect(username=user, password=password)
            sftp = paramiko.SFTPClient.from_transport(transport)
            print("[CONNECT] SFTP 連線成功")
            return transport, sftp
        except Exception as e:
            print(f"[CONNECT ERROR] {e}")
            print("[RETRY] 5 秒後重試...")
            time.sleep(5)

# =====================================================
#               主循環
# =====================================================
def main_loop(host, port, user, password, remote_dir, local_dir, sync_subdirs):
    transport, sftp = connect_sftp(host, port, user, password)
    print(f"[INFO][{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] SFTP 同步啟動")

    while True:
        try:
            print(f"\n[SCAN] 掃描 SFTP 目錄：{remote_dir}")
            sftp.listdir(remote_dir)  # 測試連線是否還活著
            sftp_download(sftp, remote_dir, local_dir, sync_subdirs)

            print(f"[SCAN DONE][{datetime.now().strftime('%H:%M:%S')}]")

        except Exception as e:
            print(f"[SFTP ERROR] {e}")
            try:
                transport.close()
            except:
                pass

            print("[RECONNECT] 連線中斷，重新連線...")
            transport, sftp = connect_sftp(
                host, port, user, password
            )

        time.sleep(SCAN_INTERVAL)

# =====================================================
#                     主程式
# =====================================================
if __name__ == "__main__":
    host, port, user, passwd, remote_dir, local_dir, sync_subdirs = get_config()
    os.makedirs(local_dir, exist_ok=True)

    try:
        main_loop(host, port, user, passwd, remote_dir, local_dir, sync_subdirs)
    except KeyboardInterrupt:
        print("\n[EXIT] 使用者中斷")
    except Exception as e:
        print(f"[FATAL ERROR] {e}")
