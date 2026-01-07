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
CONFIG_FILE = os.path.join(BASE_DIR, "config_ftp_d.ini")
disnamelen = 23
extranamelen = 89
SCAN_INTERVAL = 5

# =====================================================
#                   讀取 config
# =====================================================
def get_config():
    cfg = configparser.ConfigParser()

    default_cfg = {
        "FTP": {
            "host": "127.0.0.1",
            "port": "22",
            "user": "",
            "password": "",
            "folder": "/"
        },
        "LOCAL": {
            "download_folder": "ftp_download"
        }
    }

    if not os.path.exists(CONFIG_FILE):
        cfg.read_dict(default_cfg)
        with open(CONFIG_FILE, "w", encoding="utf-8") as f:
            cfg.write(f)
        print("[CONFIG] 已建立預設設定檔")
    else:
        cfg.read(CONFIG_FILE, encoding="utf-8")

    return (
        cfg.get("FTP", "host"),
        cfg.getint("FTP", "port"),
        cfg.get("FTP", "user"),
        cfg.get("FTP", "password"),
        cfg.get("FTP", "folder"),
        os.path.join(BASE_DIR, cfg.get("LOCAL", "download_folder"))
    )

# =====================================================
#               SFTP 遞迴下載
# =====================================================
def sftp_download_all(sftp, remote_dir, local_dir):
    os.makedirs(local_dir, exist_ok=True)

    try:
        items = sftp.listdir_attr(remote_dir)
    except Exception as e:
        print(f"[ERROR] 無法存取目錄：{remote_dir} - {e}")
        return

    for attr in items:
        r_path = f"{remote_dir}/{attr.filename}".replace("//", "/")
        l_path = os.path.join(local_dir, attr.filename)

        # 目錄
        if S_ISDIR(attr.st_mode):
            sftp_download_all(sftp, r_path, l_path)
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

        def progress(transferred, total):
            nonlocal downloaded
            downloaded = transferred
            elapsed = time.time() - start_time
            speed = downloaded / (1024 * 1000) / elapsed if elapsed > 0 else 0
            eta = (total - downloaded) / (1024 * 1000) / speed if speed > 0 else 0
            mm, ss = divmod(int(eta), 60)
            print(
                f"\r[DOWNLOADING][SFTP] ↓ {print_name}: "
                f"{downloaded/1024/1024:.2f}/{total/1024/1024:.2f} MB "
                f"({speed:.2f} MB/s, {mm:02d}:{ss:02d})",
                end="", flush=True
            )

        try:
            sftp.get(r_path, l_path + ".tmp", callback=progress)
            os.replace(l_path + ".tmp", l_path)
        except Exception as e:
            if os.path.exists(l_path + ".tmp"):
                os.remove(l_path + ".tmp")
            print(f"\n[DOWNLOAD ERROR] {r_path} - {e}")
            continue

        print(
            f"\n[DOWNLOAD DONE][{datetime.now().strftime('%H:%M:%S')}] "
            f"{l_path} ({remote_size/1024/1024:.2f} MB)"
        )

# =====================================================
#               SFTP 連線與重連
# =====================================================
def connect_sftp(host, port, user, password):
    print(f"[CONNECT] SFTP {host}:{port}")
    transport = paramiko.Transport((host, port))
    transport.connect(username=user, password=password)
    sftp = paramiko.SFTPClient.from_transport(transport)
    return transport, sftp

# =====================================================
#               主循環
# =====================================================
def main_loop(host, port, user, password, remote_dir, local_dir):
    transport, sftp = connect_sftp(host, port, user, password)

    while True:
        try:
            print(f"\n[SCAN] {remote_dir}")
            sftp_download_all(sftp, remote_dir, local_dir)
            print(f"[SCAN DONE][{datetime.now().strftime('%H:%M:%S')}]")
        except Exception as e:
            print(f"[SFTP ERROR] {e}")
            try:
                transport.close()
            except:
                pass
            time.sleep(5)
            transport, sftp = connect_sftp(host, port, user, password)

        time.sleep(SCAN_INTERVAL)

# =====================================================
#                     主程式
# =====================================================
if __name__ == "__main__":
    host, port, user, passwd, r_dir, l_dir = get_config()
    os.makedirs(l_dir, exist_ok=True)

    try:
        main_loop(host, port, user, passwd, r_dir, l_dir)
    except KeyboardInterrupt:
        print("\n[EXIT] 使用者中斷")
