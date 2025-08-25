import subprocess
import threading
import time
from pathlib import Path
from datetime import datetime

# 計算資料夾總大小
def get_folder_size(path: Path):
    total = 0
    for file in path.rglob("*"):
        if file.is_file():
            total += file.stat().st_size
    return total

# 壓縮來源資料夾
source_folder = Path(r"C:\Users\sgrc - 325\Desktop\seismograph data\old")

# 生成日期壓縮檔名
today_str = datetime.now().strftime("%d%m%y")
final_file = Path(fr"C:\Users\sgrc - 325\Desktop\seismograph data\old_{today_str}.7z")
tmp_file = final_file.with_suffix(".7z.tmp")

# 如果有舊檔，先刪除
if final_file.exists():
    final_file.unlink()
if tmp_file.exists():
    tmp_file.unlink()

# 總大小 (bytes)
total_size = get_folder_size(source_folder)
print(f"來源資料夾總大小: {total_size / (1024*1024):.2f} MB")

# 檔案大小監控 (顯示 ETA)
def monitor_file_size(path: Path, stop_flag: list, start_time, total_size):
    while not stop_flag[0]:
        if path.exists():
            current_size = path.stat().st_size
            elapsed = time.time() - start_time

            size_mb = current_size / (1024 * 1024)
            print(f"[進度] {elapsed:.1f} s , 目前大小 : {size_mb:.2f} MB")

        time.sleep(2)  # 每 2 秒更新一次

# 7-Zip 壓縮命令 (輸出到 .7z.tmp)
cmd = [
    r"C:\Program Files\7-Zip\7z.exe",
    "a",
    str(tmp_file),
    str(source_folder),
    "-t7z",
    "-mx=7",
    "-m0=LZMA2",
    "-mfb=273",
    # "-md=1024m",
    # "-mmt=10",
    "-md=512m",
    "-mmt=18",
    "-y"
]

# 啟動監控
stop_flag = [False]
start_time = time.time()
monitor_thread = threading.Thread(target=monitor_file_size, args=(tmp_file, stop_flag, start_time, total_size))
monitor_thread.start()

try:
    # 執行壓縮
    subprocess.run(cmd, check=True)
    print("壓縮完成！")

    # 改名成 .7z
    tmp_file.rename(final_file)
    print(f"檔案已更名為: {final_file}")

finally:
    stop_flag[0] = True
    monitor_thread.join()
