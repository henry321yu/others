import subprocess
import threading
import time
from pathlib import Path
from datetime import datetime

# 格式化檔案大小
def format_size(size_bytes):
    if size_bytes >= 1024 ** 3:
        return f"{size_bytes / (1024 ** 3):.2f} GB"
    elif size_bytes >= 1024 ** 2:
        return f"{size_bytes / (1024 ** 2):.2f} MB"
    elif size_bytes >= 1024:
        return f"{size_bytes / 1024:.2f} KB"
    else:
        return f"{size_bytes} Bytes"

# 格式化時間 (秒 -> mm:ss)
def format_time(seconds):
    if seconds <= 0:
        return "0s"
    m, s = divmod(int(seconds), 60)
    if m > 60:
        h, m = divmod(m, 60)
        return f"{h}h {m}m {s}s"
    return f"{m}m {s}s"

# 計算資料夾總大小
def get_folder_size(path: Path):
    total = 0
    for file in path.rglob("*"):
        if file.is_file():
            total += file.stat().st_size
    return total

# 生成日期壓縮檔名
today_str = datetime.now().strftime("%y%m%d")

# # 壓縮來源資料夾
# source_folder = Path(r"C:\Users\sgrc - 325\Desktop\seismograph data\old")
# final_file = Path(fr"C:\Users\sgrc - 325\Desktop\seismograph data\old_{today_str}.7z")
# 壓縮來源資料夾
source_folder = Path(r"D:\fly_data")
final_file = Path(fr"D:\fly_data_{today_str}.7z")

tmp_file = final_file.with_suffix(".7z.tmp")

# 如果有舊檔，先刪除
if final_file.exists():
    final_file.unlink()
if tmp_file.exists():
    tmp_file.unlink()

# 計算來源資料夾總大小
total_size = get_folder_size(source_folder)
print(f"來源資料夾總大小: {format_size(total_size)}")

# 檔案大小監控 (顯示進度 + ETA)
def monitor_file_size(path: Path, stop_flag: list, start_time, total_size):
    while not stop_flag[0]:
        if path.exists():
            current_size = path.stat().st_size
            elapsed = time.time() - start_time

            # 計算剩餘時間
            if current_size > 0:
                estimated_total_time = (total_size / current_size) * elapsed
                remaining_time = estimated_total_time - elapsed
                percent = (current_size / total_size) * 100
            else:
                remaining_time = 0
                percent = 0

            print(
                f" | {format_time(elapsed)} | "
                f"目前大小 {format_size(current_size)}"
            )

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
    # "-mmt=8",
    "-md=512m",
    "-mmt=18",
    "-y"
]

# 啟動監控
stop_flag = [False]
start_time = time.time()
monitor_thread = threading.Thread(
    target=monitor_file_size,
    args=(tmp_file, stop_flag, start_time, total_size)
)
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
