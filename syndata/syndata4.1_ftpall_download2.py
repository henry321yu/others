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

# 顯示相關常數（與你上傳程式風格一致）
disnamelen = 23
extranamelen = 89
SCAN_INTERVAL = 5


# =====================================================
#                   讀取 FTP 設定
# =====================================================
def get_ftp_config():
    """從 config_download.ini 讀取或建立 FTP 設定"""
    config = configparser.ConfigParser()
    if os.path.exists(CONFIG_FILE):
        config.read(CONFIG_FILE, encoding="utf-8")

    if not config.has_section("FTP"):
        config.add_section("FTP")

    default_values = {
        "host": "192.168.138.207",
        "user": "admin",
        "password": "123",
        "folder": "/"   # 遠端要下載的資料夾
    }

    updated = False
    for k, v in default_values.items():
        if not config.has_option("FTP", k):
            config.set("FTP", k, v)
            updated = True

    if not config.has_section("LOCAL"):
        config.add_section("LOCAL")
        config.set("LOCAL", "download_folder", "download_data")
        updated = True
    else:
        if not config.has_option("LOCAL", "download_folder"):
            config.set("LOCAL", "download_folder", "download_data")
            updated = True

    if updated:
        with open(CONFIG_FILE, "w", encoding="utf-8") as f:
            config.write(f)

    return (
        config.get("FTP", "host").strip(),
        config.get("FTP", "user").strip(),
        config.get("FTP", "password").strip(),
        config.get("FTP", "folder").strip()
    )


# =====================================================
#            讀取本地下載資料夾 (config)
# =====================================================
def get_download_folder():
    config = configparser.ConfigParser()
    if os.path.exists(CONFIG_FILE):
        config.read(CONFIG_FILE, encoding="utf-8")

    if not config.has_section("LOCAL"):
        config.add_section("LOCAL")

    if config.has_option("LOCAL", "download_folder"):
        folder_name = config.get("LOCAL", "download_folder").strip() or "download_data"
    else:
        folder_name = "download_data"
        config.set("LOCAL", "download_folder", folder_name)
        with open(CONFIG_FILE, "w", encoding="utf-8") as f:
            config.write(f)

    full_path = os.path.join(BASE_DIR, folder_name)
    os.makedirs(full_path, exist_ok=True)
    return full_path


# =====================================================
#        safe_nlst: 用 retrbinary 取得 raw bytes 列表
# =====================================================
def safe_decode_name(b: bytes) -> str:
    """嘗試以多種編碼 decode bytes 成字串（utf-8 -> cp950 -> cp1252(replace)）"""
    if isinstance(b, str):
        return b
    for enc in ("utf-8", "cp950", "cp1252"):
        try:
            return b.decode(enc)
        except Exception:
            continue
    # 最後 fallback 用 latin-1 直接映射
    try:
        return b.decode("latin-1", errors="replace")
    except Exception:
        return str(b)


def safe_nlst(ftp: ftplib.FTP):
    """
    安全取得當前目錄下的項目名（避免 ftplib 自動以錯誤編碼 decode 而 crash）。
    回傳一個 list of str（已 decode）
    """
    buf = bytearray()

    def handle_bytes(data: bytes):
        buf.extend(data)

    try:
        # retrbinary("NLST") 會把列出的文字當 raw bytes 傳回
        ftp.retrbinary("NLST", handle_bytes)
    except Exception:
        # 部分 FTP 伺服器可能拒絕 NLST binary 方式
        # fallback to normal nlst but guarded
        try:
            items = ftp.nlst()
            # nlst 可能已經是 str list
            return [i if isinstance(i, str) else safe_decode_name(i) for i in items]
        except Exception:
            return []

    # split by CRLF
    raw_lines = buf.split(b"\r\n")
    items = []
    for line in raw_lines:
        if not line:
            continue
        items.append(safe_decode_name(line.strip()))
    return items


# =====================================================
#             協助函式：嘗試以多種方式執行命令
# =====================================================
def try_cwd(ftp: ftplib.FTP, path: str) -> bool:
    """嘗試 cd，若失敗嘗試用 utf8->latin1 轉換再試一次"""
    try:
        ftp.cwd(path)
        return True
    except Exception:
        try:
            alt = path.encode("utf-8", errors="ignore").decode("latin-1", errors="ignore")
            ftp.cwd(alt)
            return True
        except Exception:
            return False


def try_size(ftp: ftplib.FTP, path: str):
    """嘗試得到 size，若失敗嘗試轉碼後再取"""
    try:
        return ftp.size(path)
    except Exception:
        try:
            alt = path.encode("utf-8", errors="ignore").decode("latin-1", errors="ignore")
            return ftp.size(alt)
        except Exception:
            return -1


def try_retrbinary(ftp: ftplib.FTP, path: str, callback):
    """嘗試 retrbinary，若失敗嘗試用 utf8->latin1 編碼再執行"""
    try:
        ftp.retrbinary(f"RETR {path}", callback)
        return True
    except Exception:
        try:
            alt = path.encode("utf-8", errors="ignore").decode("latin-1", errors="ignore")
            ftp.retrbinary(f"RETR {alt}", callback)
            return True
        except Exception:
            return False


# =====================================================
#              顯示與下載主函式（遞迴）
# =====================================================
def ftp_download_all(ftp: ftplib.FTP, remote_folder: str, local_folder: str):
    """
    遞迴下載 remote_folder 內容到 local_folder
    顯示風格盡量與上傳程式一致
    """
    # 嘗試切換到 remote_folder（支援多種編碼嘗試）
    if not try_cwd(ftp, remote_folder):
        print(f"[ERROR] FTP 資料夾不存在或無法存取：{remote_folder}")
        return

    os.makedirs(local_folder, exist_ok=True)

    items = safe_nlst(ftp)

    for item in items:
        # item 是名子（檔名或子資料夾）
        item_remote = f"{remote_folder}/{item}".replace("//", "/")
        item_local = os.path.join(local_folder, item)

        # 判斷是否為資料夾（用 try_cwd）
        is_dir = False
        if try_cwd(ftp, item_remote):
            # 是資料夾，回到 parent（remote_folder）
            try:
                ftp.cwd(remote_folder)
            except Exception:
                pass
            is_dir = True

        if is_dir:
            ftp_download_all(ftp, item_remote, item_local)
            continue

        # 檔案：先取得遠端大小（嘗試多種編碼）
        remote_size = try_size(ftp, item_remote)
        if remote_size is None:
            remote_size = -1

        # 若本地已存在且大小相同 → 跳過
        if os.path.exists(item_local) and remote_size != -1 and os.path.getsize(item_local) == remote_size:
            print(f"[SKIP EXISTING] {item_local}")
            continue

        # 開始下載並顯示進度
        megabyte = 1024 * 1000
        print_name = (item[:disnamelen - 3] + '...') if len(item) > disnamelen else item
        start_time = time.time()
        downloaded = 0

        # 確保上層資料夾存在
        os.makedirs(os.path.dirname(item_local) or ".", exist_ok=True)

        # 使用 retrbinary 並在 callback 中寫入並更新進度
        def make_callback(fobj):
            def callback(chunk):
                nonlocal downloaded, start_time
                fobj.write(chunk)
                downloaded += len(chunk)
                elapsed = time.time() - start_time
                speed = downloaded / megabyte / elapsed if elapsed > 0 else 0
                eta = (remote_size - downloaded) / megabyte / speed if (remote_size > 0 and speed > 0) else 0
                hh = int(eta // 3600)
                mm = int((eta % 3600) // 60)
                ss = int(eta % 60)
                if hh > 0:
                    eta_str = f"{hh:d}:{mm:02d}:{ss:02d}"
                else:
                    eta_str = f"{mm:02d}:{ss:02d}"

                # 與上傳程式排版一致
                print(f"\r[DOWNLOADING][FTP] ↓ {print_name}:{(remote_size / megabyte) if remote_size>0 else 0:.2f} MB/{downloaded / megabyte:.2f} MB"
                      f"({downloaded / remote_size*100:.2f}%, {speed:.2f} MB/S, {eta_str})" if remote_size>0 else
                      f"\r[DOWNLOADING][FTP] ↓ {print_name}:{downloaded / megabyte:.2f} MB ( {speed:.2f} MB/S )",
                      end='', flush=True)
            return callback

        # open file and call retrbinary (with fallback encoding attempts)
        try:
            with open(item_local + ".tmp", "wb") as f_local:
                cb = make_callback(f_local)
                success = try_retrbinary(ftp, item_remote, cb)
                if not success:
                    # failed to download
                    raise Exception("RETR failed (all encodings tried)")
        except Exception as e:
            # 清理不完整檔案
            try:
                if os.path.exists(item_local + ".tmp"):
                    os.remove(item_local + ".tmp")
            except Exception:
                pass
            print('\r' + ' ' * (disnamelen + extranamelen) + '\r', end='')   # 清除行
            print(f"[DOWNLOAD ERROR][{datetime.now().strftime('%H:%M:%S')}] {item_remote} - {e}")
            continue

        # 下載完畢：改名並顯示完成訊息
        try:
            if os.path.exists(item_local):
                os.remove(item_local)
            os.rename(item_local + ".tmp", item_local)
        except Exception as e:
            print(f"[CLEANUP ERROR] 無法最終命名檔案 {item_local} - {e}")
        print('\r' + ' ' * (disnamelen + extranamelen) + '\r', end='')   # 清除行
        print(f"[DOWNLOAD DONE][{datetime.now().strftime('%H:%M:%S')}] {item_local} ({remote_size / megabyte:.2f} MB)")


    print(f"[DONE] {remote_folder}")


# =====================================================
#                     主程式執行
# =====================================================
if __name__ == "__main__":
    ftp_host, ftp_user, ftp_pass, ftp_folder = get_ftp_config()
    download_folder = get_download_folder()

    ftp = ftplib.FTP()
    # 不強制用 utf-8 解碼（我們在 safe_nlst 中處理 decode）
    # ftp.encoding = "utf-8"

    print(f"\n[CONNECT] 連線到 FTP: {ftp_host} ...")
    ftp.connect(ftp_host, 21, timeout=10)
    ftp.login(ftp_user, ftp_pass)

    print(f"[START DOWNLOAD] 從 FTP {ftp_folder} ↓ 本地 {download_folder}\n")

    ftp_download_all(ftp, ftp_folder, download_folder)

    try:
        ftp.quit()
    except Exception:
        # 某些情形下 server 可能已中斷連線
        pass

    print("\n[ALL DOWNLOAD COMPLETED]")
