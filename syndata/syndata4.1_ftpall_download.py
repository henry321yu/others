import os
import ftplib
import configparser

CONFIG_FILE = "config.ini"
BASE_DIR = os.path.dirname(os.path.abspath(__file__))

# =====================================================
#                   讀取 FTP 設定
# =====================================================
def get_ftp_config():
    config = configparser.ConfigParser()
    if os.path.exists(CONFIG_FILE):
        config.read(CONFIG_FILE, encoding="utf-8")

    if not config.has_section("FTP"):
        config.add_section("FTP")

    defaults = {
        "host": "192.168.138.207",
        "user": "admin",
        "password": "123",
        "folder": "/"  # FTP 遠端資料夾
    }
    updated = False

    for key, val in defaults.items():
        if not config.has_option("FTP", key):
            config.set("FTP", key, val)
            updated = True

    if updated:
        with open(CONFIG_FILE, "w", encoding="utf-8") as f:
            config.write(f)

    return (
        config.get("FTP", "host"),
        config.get("FTP", "user"),
        config.get("FTP", "password"),
        config.get("FTP", "folder")
    )

# =====================================================
#            設定本地下載資料夾 (config.ini)
# =====================================================
def get_download_folder():
    config = configparser.ConfigParser()
    if os.path.exists(CONFIG_FILE):
        config.read(CONFIG_FILE, encoding="utf-8")

    if not config.has_section("LOCAL"):
        config.add_section("LOCAL")

    if config.has_option("LOCAL", "download_folder"):
        folder_name = config.get("LOCAL", "download_folder")
    else:
        folder_name = "download_data"
        config.set("LOCAL", "download_folder", folder_name)
        with open(CONFIG_FILE, "w", encoding="utf-8") as f:
            config.write(f)

    full_path = os.path.join(BASE_DIR, folder_name)
    os.makedirs(full_path, exist_ok=True)
    return full_path

# =====================================================
#               遞迴下載 FTP 資料夾
# =====================================================
def ftp_download_all(ftp, remote_folder, local_folder):
    try:
        ftp.cwd(remote_folder)
    except:
        print(f"[ERROR] FTP 資料夾不存在：{remote_folder}")
        return

    os.makedirs(local_folder, exist_ok=True)

    try:
        items = ftp.nlst()
    except Exception as e:
        print(f"[ERROR] 無法列出：{remote_folder} → {e}")
        return

    for item in items:
        item_remote = f"{remote_folder}/{item}".replace("//", "/")
        item_local = os.path.join(local_folder, item)

        # 判斷是否為資料夾
        is_dir = False
        try:
            ftp.cwd(item_remote)
            ftp.cwd("..")
            is_dir = True
        except:
            is_dir = False

        # ───────────────────────
        #   子資料夾處理
        # ───────────────────────
        if is_dir:
            ftp_download_all(ftp, item_remote, item_local)
            continue

        # ───────────────────────
        #   檔案下載
        # ───────────────────────
        try:
            remote_size = ftp.size(item_remote)
        except:
            remote_size = -1

        # 若本地已有同大小 → 跳過
        if os.path.exists(item_local) and os.path.getsize(item_local) == remote_size:
            print(f"[SKIP EXISTING] {item_local}")
            continue

        print(f"[DOWNLOAD] {item_remote} → {item_local}")

        with open(item_local, "wb") as f:
            ftp.retrbinary(f"RETR {item_remote}", f.write)

    print(f"[DONE] {remote_folder}")

# =====================================================
#                     主程式執行
# =====================================================
if __name__ == "__main__":

    ftp_host, ftp_user, ftp_pass, ftp_folder = get_ftp_config()
    download_folder = get_download_folder()

    ftp = ftplib.FTP()
    ftp.encoding = "utf-8"

    print(f"\n[CONNECT] 連線到 FTP: {ftp_host} ...")
    ftp.connect(ftp_host, 21, timeout=10)
    ftp.login(ftp_user, ftp_pass)

    print(f"[START DOWNLOAD] 從 FTP {ftp_folder} ↓ 本地 {download_folder}\n")

    ftp_download_all(ftp, ftp_folder, download_folder)

    ftp.quit()
    print("\n[ALL DOWNLOAD COMPLETED]")
