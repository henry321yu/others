import os
import time
import configparser
import ftplib
from datetime import datetime

# =====================================================
#                     基本設定
# =====================================================
BASE_DIR = os.path.dirname(os.path.abspath(__file__))
CONFIG_FILE = os.path.join(BASE_DIR, "config_download.ini")
disnamelen = 23
extranamelen = 89
SCAN_INTERVAL = 5

# =====================================================
#                   讀取 config
# =====================================================
def get_config():
    cfg = configparser.ConfigParser()
    if os.path.exists(CONFIG_FILE):
        cfg.read(CONFIG_FILE, encoding="utf-8")
    if not cfg.has_section("FTP"):
        cfg.add_section("FTP")
    if not cfg.has_section("LOCAL"):
        cfg.add_section("LOCAL")
    defaults = {
        "host": "192.168.138.207",
        "user": "admin",
        "password": "123",
        "folder": "/",
        "download_folder": "download_data"
    }
    updated = False
    for k, v in defaults.items():
        sec = "LOCAL" if k=="download_folder" else "FTP"
        if not cfg.has_option(sec, k):
            cfg.set(sec, k, v)
            updated = True
    if updated:
        with open(CONFIG_FILE, "w", encoding="utf-8") as f:
            cfg.write(f)
    return (
        cfg.get("FTP", "host").strip(),
        cfg.get("FTP", "user").strip(),
        cfg.get("FTP", "password").strip(),
        cfg.get("FTP", "folder").strip(),
        os.path.join(BASE_DIR, cfg.get("LOCAL", "download_folder").strip())
    )

# =====================================================
#             安全列出 FTP 目錄
# =====================================================
def safe_nlst(ftp):
    buf = bytearray()
    try:
        ftp.retrbinary("NLST", buf.extend)
        lines = [b.decode("utf-8", errors="ignore") for b in buf.split(b"\r\n") if b.strip()]
    except:
        try:
            lines = ftp.nlst()
        except:
            lines = []
    return lines

# =====================================================
#             FTP 編碼轉換與 size
# =====================================================
def try_cwd(ftp, path):
    try:
        ftp.cwd(path)
        return True
    except:
        try:
            ftp.cwd(path.encode("utf-8").decode("latin-1"))
            return True
        except:
            return False

def try_size(ftp, path):
    try:
        return ftp.size(path)
    except:
        try:
            return ftp.size(path.encode("utf-8").decode("latin-1"))
        except:
            return -1

def try_retrbinary(ftp, path, callback):
    try:
        ftp.retrbinary(f"RETR {path}", callback)
        return True
    except:
        try:
            ftp.retrbinary(f"RETR {path.encode('utf-8').decode('latin-1')}", callback)
            return True
        except:
            return False

# =====================================================
#           遞迴下載主函式
# =====================================================
# =====================================================
#           下載 FTP 根目錄檔案（不遞迴子資料夾）
# =====================================================
def ftp_download(ftp, remote_folder, local_folder):
    if not try_cwd(ftp, remote_folder):
        print(f"[ERROR] FTP 資料夾不存在：{remote_folder}")
        return
    os.makedirs(local_folder, exist_ok=True)
    items = safe_nlst(ftp)

    for item in items:
        item_remote = f"{remote_folder}/{item}".replace("//", "/")
        item_local = os.path.join(local_folder, item)

        # 檢查是否為資料夾
        if try_cwd(ftp, item_remote):
            # 是資料夾就跳過，不遞迴
            print(f"[SKIP DIR] {item_remote}")
            ftp.cwd(remote_folder)  # 返回上層
            continue

        remote_size = try_size(ftp, item_remote)
        if os.path.exists(item_local) and remote_size > 0 and os.path.getsize(item_local)==remote_size:
            print(f"[SKIP EXISTING] {item_local}")
            continue

        print_name = (item[:disnamelen-3]+"...") if len(item)>disnamelen else item
        start_time = time.time()
        downloaded = 0
        os.makedirs(os.path.dirname(item_local) or ".", exist_ok=True)

        def callback(chunk):
            nonlocal downloaded, start_time
            f.write(chunk)
            downloaded += len(chunk)
            elapsed = time.time()-start_time
            speed = downloaded/(1024*1000)/elapsed if elapsed>0 else 0
            eta = (remote_size-downloaded)/(1024*1000)/speed if remote_size>0 and speed>0 else 0
            hh, mm, ss = int(eta//3600), int((eta%3600)//60), int(eta%60)
            eta_str = f"{hh:d}:{mm:02d}:{ss:02d}" if hh>0 else f"{mm:02d}:{ss:02d}"
            print(f"\r[DOWNLOADING][FTP] ↓ {print_name}:{(remote_size/1024/1024) if remote_size>0 else 0:.2f}MB/{downloaded/1024/1024:.2f}MB"
                  f"({downloaded/remote_size*100:.2f}%, {speed:.2f} MB/S, {eta_str})" if remote_size>0 else
                  f"\r[DOWNLOADING][FTP] ↓ {print_name}:{downloaded/1024/1024:.2f}MB ({speed:.2f} MB/S)",
                  end="", flush=True)

        try:
            with open(item_local+".tmp","wb") as f:
                if not try_retrbinary(ftp, item_remote, callback):
                    raise Exception("RETR failed")
        except Exception as e:
            if os.path.exists(item_local+".tmp"):
                os.remove(item_local+".tmp")
            print('\r'+' '* (disnamelen+extranamelen)+'\r', end='')
            print(f"[DOWNLOAD ERROR][{datetime.now().strftime('%H:%M:%S')}] {item_remote} - {e}")
            continue

        try:
            if os.path.exists(item_local):
                os.remove(item_local)
            os.rename(item_local+".tmp", item_local)
        except:
            pass
        print('\r'+' '* (disnamelen+extranamelen)+'\r', end='')
        print(f"[DOWNLOAD DONE][{datetime.now().strftime('%H:%M:%S')}] {item_local} ({remote_size/1024/1024:.2f} MB)")

    print(f"[DONE] {remote_folder}")


# =====================================================
#           持續掃描並下載
# =====================================================
def scan_and_download(ftp, remote_folder, local_folder):
    print(f"\n[SCAN] 開始掃描 FTP 資料夾: {remote_folder}")
    ftp_download(ftp, remote_folder, local_folder)
    print(f"[SCAN DONE][{datetime.now().strftime('%H:%M:%S')}]")

def main_loop(ftp, ftp_folder, download_folder):
    print(f"[INFO][{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] FTP 同步啟動")
    while True:
        scan_and_download(ftp, ftp_folder, download_folder)
        time.sleep(SCAN_INTERVAL)

# =====================================================
#                     主程式
# =====================================================
if __name__=="__main__":
    ftp_host, ftp_user, ftp_pass, ftp_folder, download_folder = get_config()
    print(f"\n[CONNECT] 連線到 FTP: {ftp_host} ...")
    ftp = ftplib.FTP()
    ftp.connect(ftp_host,21,timeout=10)
    ftp.login(ftp_user,ftp_pass)
    os.makedirs(download_folder, exist_ok=True)

    try:
        main_loop(ftp, ftp_folder, download_folder)
    except KeyboardInterrupt:
        print("\n[EXIT] 使用者中斷")
    except Exception as e:
        print(f"[FATAL ERROR] {e}")
    finally:
        try:
            ftp.quit()
        except:
            pass
