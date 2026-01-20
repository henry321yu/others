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
CONFIG_FILE = os.path.join(BASE_DIR, "config_sftp.ini")
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
            "sync_subdirs": "yes",
            "mode": "download"
        },
        "LOCAL": {
            "local_folder": "sftp_sync"
        }
    }

    if not os.path.exists(CONFIG_FILE):
        cfg.read_dict(default_cfg)
        with open(CONFIG_FILE, "w", encoding="utf-8") as f:
            cfg.write(f)
        log("[CONFIG] 已建立預設設定檔")
        log(f"[CONFIG] 請編輯設定檔後儲存：{CONFIG_FILE}")

        last_mtime = os.path.getmtime(CONFIG_FILE)

        while True:
            time.sleep(3)
            log(f"[WAIT] 等待使用者編輯設定檔：{CONFIG_FILE}")

            try:
                current_mtime = os.path.getmtime(CONFIG_FILE)
            except FileNotFoundError:
                continue

            if current_mtime != last_mtime:
                log("[CONFIG] 偵測到設定檔變更，重新讀取")
                cfg.read(CONFIG_FILE, encoding="utf-8")
                break
    else:
        cfg.read(CONFIG_FILE, encoding="utf-8")

    sync_subdirs = cfg.get("SFTP", "sync_subdirs", fallback="yes").lower() == "yes"
    mode = cfg.get("SFTP", "mode", fallback="download").lower()
    if mode not in ("upload", "download"):        
        while True:
            time.sleep(3)
            log("[CONFIG ERROR] mode 必須是 upload 或 download")
            log(f"[CONFIG ERROR] 等待使用者編輯設定檔：{CONFIG_FILE}")
            try:
                current_mtime = os.path.getmtime(CONFIG_FILE)
            except FileNotFoundError:
                continue

            if current_mtime != last_mtime:
                log("[CONFIG] 偵測到設定檔變更，重新讀取")
                cfg.read(CONFIG_FILE, encoding="utf-8")
                break

    return (
        cfg.get("SFTP", "host"),
        cfg.getint("SFTP", "port"),
        cfg.get("SFTP", "user"),
        cfg.get("SFTP", "password"),
        cfg.get("SFTP", "target_folder"),
        os.path.join(BASE_DIR, cfg.get("LOCAL", "local_folder")),
        sync_subdirs,
        mode
    )

def log(msg):
    ts = datetime.now()
    ms = int(ts.microsecond / 10000)
    # line = f"[{ts.strftime('%Y-%m-%d %H:%M:%S')}.{ms:02d}] {msg}"
    line = f"[{ts.strftime('%H:%M:%S')}] {msg}"
    print(line)

# =====================================================
#             SFTP 遞迴下載\上傳主函式
# =====================================================
def format_size(size_bytes):
    """自動將大小轉換為 B/KB/MB/GB"""
    if size_bytes < 1024:
        return f"{size_bytes:.1f} bytes"
    elif size_bytes < 1024**2:
        return f"{size_bytes/1024:.1f} KB"
    elif size_bytes < 1024**3:
        return f"{size_bytes/1024**2:.1f} MB"
    else:
        return f"{size_bytes/1024**3:.2f} GB"

def sftp_download(sftp, remote_dir, local_dir, items, sync_subdirs, scanned_cache, remote_file_count):
    os.makedirs(local_dir, exist_ok=True)
    print_count = False

    for attr in items:
        r_path = f"{remote_dir}/{attr.filename}".replace("//", "/")
        l_path = os.path.join(local_dir, attr.filename) 

        # ---------- 目錄 ----------
        if S_ISDIR(attr.st_mode):
            if sync_subdirs:
                try:
                    if print_count:
                        print('\r' + ' ' * (disnamelen + extranamelen) + '\r', end='', flush=True)
                        print_count = False

                    sub_items = listdir_with_progress(sftp,r_path,label="遠端子目錄")
                    remote_file_count["done"] = 0
                    remote_file_count["total"] = len(sub_items)

                    sftp_download(
                        sftp,
                        r_path,
                        l_path,
                        sub_items,
                        sync_subdirs,
                        scanned_cache,
                        remote_file_count
                    )

                except Exception as e:
                    log(f"[DIR ERROR] {r_path} - {e}")
            continue
        remote_size = attr.st_size

        remote_file_count["done"] += 1
        print_progress(
            remote_file_count["done"],
            remote_file_count["total"],
            label="DOWNLOADING"
        )
        print_count = True

        file_key = (r_path, attr.st_size)
        if file_key in scanned_cache:
            continue

        scanned_cache.add(file_key)

        if os.path.exists(l_path) and os.path.getsize(l_path) == attr.st_size:
            if print_count:
                print('\r' + ' ' * (disnamelen + extranamelen) + '\r', end='', flush=True)
                print_count = False

            log(f"[SKIP DOWNLOADING] {l_path}")
            continue

        print_name = (
            attr.filename[:disnamelen-3] + "..."
            if len(attr.filename) > disnamelen else attr.filename
        )

        start_time = time.time()
        last_len = 0

        def progress(transferred, total):
            nonlocal last_len
            elapsed = time.time() - start_time
            speed = transferred / elapsed if elapsed > 0 else 0
            percent = transferred / total * 100 if total > 0 else 0
            eta = (total - transferred) / speed if speed > 0 else 0
            mm, ss = divmod(int(eta), 60)

            line = (
                f"[DOWNLOADING] ↓ {print_name}: "
                f"{format_size(transferred)}/{format_size(total)} "
                f"({percent:.2f}%, {format_size(speed)}/s, {mm:02d}:{ss:02d})"
            )

            pad = max(last_len - len(line), 0)
            print('\r' + line + ' ' * pad, end='', flush=True)
            last_len = len(line)

        tmp_path = l_path + ".tmp"

        try:
            sftp.get(r_path, tmp_path, callback=progress)
            os.replace(tmp_path, l_path)      

            if print_count:
                print('\r' + ' ' * (disnamelen + extranamelen) + '\r', end='', flush=True)
                print_count = False                

            print('\r' + ' ' * (disnamelen + extranamelen) + '\r', end='')
            log(f"[DOWNLOAD DONE] {l_path} ({format_size(remote_size)})")

        except Exception as e:
            if os.path.exists(tmp_path):
                os.remove(tmp_path)

            if print_count:
                print('\r' + ' ' * (disnamelen + extranamelen) + '\r', end='', flush=True)
                print_count = False

            print('\r' + ' ' * (disnamelen + extranamelen) + '\r', end='')
            log(f"[DOWNLOAD ERROR] {r_path} - {e}")
    if print_count:
        print('\r' + ' ' * (disnamelen + extranamelen) + '\r', end='', flush=True)
        print_count = False     

def sftp_upload(sftp, local_dir, remote_dir, sync_subdirs, scanned_cache, total_files, processed_count):
    print_count = False

    try:
        sftp.chdir(remote_dir)
    except IOError:
        sftp.mkdir(remote_dir)
        sftp.chdir(remote_dir)

    try:
        items = os.listdir(local_dir)
    except Exception as e:
        log(f"[DIR ERROR] {local_dir} - {e}")
        return

    log(f"[SCAN DIR] 讀取本機子目錄清單中：{local_dir} ...")
    log(f"[SCAN COMPLETE] 本機子目錄讀取完成，共 {len(items)} 個檔案")

    for name in os.listdir(local_dir):
        l_path = os.path.join(local_dir, name)
        r_path = f"{remote_dir}/{name}".replace("//", "/")

        # ---------- 目錄 ----------
        if os.path.isdir(l_path):
            if sync_subdirs:
                if print_count:
                    print('\r' + ' ' * (disnamelen + extranamelen) + '\r', end='', flush=True)
                    print_count = False
                processed_count = sftp_upload(sftp, l_path, r_path, sync_subdirs, scanned_cache, total_files, processed_count)
            else:
                log(f"[SKIP DIR] {l_path}")
            continue

        local_size = os.path.getsize(l_path)
        file_key = (r_path, local_size)

        processed_count += 1
        print_progress(processed_count, total_files, label="UPLOADING")
        print_count = True

        # ---------- cache ----------
        if file_key in scanned_cache:
            continue

        try:
            r_stat = sftp.stat(r_path)
            if r_stat.st_size == local_size:
                scanned_cache.add(file_key)
                if print_count:
                    print('\r' + ' ' * (disnamelen + extranamelen) + '\r', end='', flush=True)
                    print_count = False
                log(f"[SKIP UPLOADING] {r_path}")
                continue
        except IOError:
            pass  # 遠端不存在

        print_name = (
            name[:disnamelen-3] + "..."
            if len(name) > disnamelen else name
        )

        start_time = time.time()
        last_len = 0

        def progress(transferred, total):
            nonlocal last_len
            elapsed = time.time() - start_time
            speed = transferred / elapsed if elapsed > 0 else 0
            percent = transferred / total * 100 if total > 0 else 0
            eta = (total - transferred) / speed if speed > 0 else 0
            mm, ss = divmod(int(eta), 60)

            line = (
                f"[UPLOADING] ↑ {print_name}: "
                f"{format_size(transferred)}/{format_size(total)} "
                f"({percent:.2f}%, {format_size(speed)}/s, {mm:02d}:{ss:02d})"
            )

            pad = max(last_len - len(line), 0)
            print('\r' + line + ' ' * pad, end='', flush=True)
            last_len = len(line)

        tmp_remote = r_path + ".tmp"

        try:
            sftp.put(l_path, tmp_remote, callback=progress)
            sftp.rename(tmp_remote, r_path)

            scanned_cache.add(file_key)

            print('\r' + ' ' * (disnamelen + extranamelen) + '\r', end='')
            if print_count:
                print('\r' + ' ' * (disnamelen + extranamelen) + '\r', end='', flush=True)
                print_count = False
            log(f"[UPLOAD DONE] {r_path} ({format_size(local_size)})")

        except Exception as e:
            try:
                sftp.remove(tmp_remote)
            except:
                pass

            print('\r' + ' ' * (disnamelen + extranamelen) + '\r', end='')
            if print_count:
                print('\r' + ' ' * 80 + '\r', end='', flush=True)
                print_count = False
            log(f"[UPLOAD ERROR] {l_path} - {e}")
    if print_count:
        print('\r' + ' ' * (disnamelen + extranamelen) + '\r', end='', flush=True)
        print_count = False
    return processed_count

def count_local_items(base_dir, sync_subdirs):
    count = 0
    for root, dirs, files in os.walk(base_dir):
        count += len(files)
        if not sync_subdirs:
            break
    return count

def listdir_with_progress(sftp, remote_dir, label, step=128):
    items = []
    count = 0

    if label == "遠端主目錄":
        log(f"[SCAN MAIN DIR] 讀取{label}清單中：{remote_dir} ...")
    else :
        log(f"[SCAN DIR] 讀取遠端子目錄清單中：{remote_dir} ...")

    for attr in sftp.listdir_iter(remote_dir):
        items.append(attr)
        count += 1

        if count % step == 0:
            print(f"\r[SCANING DIR] 已讀取 {count} 個檔案", end="", flush=True)

    # 清除進度列
    print('\r' + ' ' * 60 + '\r', end='')
    log(f"[SCAN COMPLETE] {label}讀取完成，共 {count} 筆")

    return items

def print_progress(processed, total, label, bar_width=30):
    percent = processed / total if total else 1
    filled = int(bar_width * percent)
    bar = "█" * filled + "-" * (bar_width - filled)
    ts = datetime.now()
    ms = int(ts.microsecond / 10000)
    print(
        f"\r[{ts.strftime('%H:%M:%S')}] [{label}] |{bar}| {percent*100:6.2f}% "
        f" {processed}/{total}",
        end="",
        flush=True
    )
    
# =====================================================
#               SFTP 連線 / 重連
# =====================================================
def connect_sftp(host, port, user, password):
    while True:
        try:
            log(f"[CONNECT] SFTP {host}:{port}")
            transport = paramiko.Transport((host, port))
            transport.connect(username=user, password=password)
            sftp = paramiko.SFTPClient.from_transport(transport)
            log("[CONNECT] SFTP 連線成功")
            return transport, sftp
        except Exception as e:
            log(f"[CONNECT ERROR] {e}")
            log("[RETRY] 5 秒後重試...")
            time.sleep(5)

# =====================================================
#               主循環
# =====================================================
def main_loop(host, port, user, password, remote_dir, local_dir, sync_subdirs, mode):
    transport, sftp = connect_sftp(host, port, user, password)

    log(f"[INFO] 同步模式：{mode.upper()}")
    log(f"[INFO] 本機目錄：{local_dir}")
    log(f"[INFO] SFTP 目錄：{remote_dir}")

    scanned_cache = set()

    while True:
        try:
            remote_file_count = {
                "total": 0,
                "done": 0
            }
            if mode == "download":
                items = listdir_with_progress(sftp,remote_dir,label="遠端主目錄")
                remote_file_count["total"] = len(items)

                sftp_download(
                    sftp,
                    remote_dir,
                    local_dir,
                    items,
                    sync_subdirs,
                    scanned_cache,
                    remote_file_count
                )
                log(f"[DOWNLOAD DONE] 遠端全數檔案下載完成，總共：{remote_file_count['total']} 個檔案")

            elif mode == "upload":
                # 確保遠端資料夾存在（你原本的程式，保持不變）
                dirs = remote_dir.replace("\\", "/").split("/")
                path = ""
                for d in dirs:
                    if not d:  # 忽略空字串
                        continue
                    path += "/" + d
                    try:
                        sftp.chdir(path)
                    except IOError:
                        sftp.mkdir(path)
                        log(f"[MKDIR] 建立遠端目錄：{path}")
                        sftp.chdir(path)

                log(f"[SCAN MAIN DIR] 讀取本機主目錄清單中：{local_dir} ...")

                total_files = count_local_items(local_dir, sync_subdirs)
                processed_count = 0

                log(f"[SCAN COMPLETE] 本機主目錄讀取完成，總共 {total_files} 個檔案")

                sftp_upload(
                    sftp,
                    local_dir,
                    remote_dir,
                    sync_subdirs,
                    scanned_cache,
                    total_files,
                    processed_count
                )

            log(f"[UPLOAD DONE][{datetime.now().strftime('%H:%M:%S')}]")

        except Exception as e:
            log(f"[SFTP ERROR] {e}")
            try:
                transport.close()
            except:
                pass

            log("[RECONNECT] 重新連線...")
            transport, sftp = connect_sftp(host, port, user, password)

        time.sleep(SCAN_INTERVAL)

# =====================================================
#                     主程式
# =====================================================
if __name__ == "__main__":
    host, port, user, passwd, remote_dir, local_dir, sync_subdirs, mode = get_config()
    os.makedirs(local_dir, exist_ok=True)

    try:
        main_loop(host, port, user, passwd, remote_dir, local_dir, sync_subdirs, mode)
    except KeyboardInterrupt:
        log("\n[EXIT] 使用者中斷")
    except Exception as e:
        log(f"[FATAL ERROR] {e}")