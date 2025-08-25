import subprocess
from pathlib import Path
from datetime import datetime

# 壓縮來源資料夾
source_folder = Path("D:/fly_data")  # 改成你的資料夾路徑

# 輸出壓縮檔名稱，帶日期
today_str = datetime.now().strftime("%d%m%y")
output_file = Path(f"D:/fly_data_{today_str}.7z")

# 7-Zip 命令（顯示詳細進度 -bb3）
cmd = [
    r"C:\Program Files\7-Zip\7z.exe",  # 7z 完整路徑
    "a",
    str(output_file),
    str(source_folder),
    "-t7z",
    "-mx=7",
    "-m0=LZMA2",
    "-md=512m",
    "-mfb=273",
    "-mmt=8",
    "-bb3"   # 顯示詳細進度
]

# 執行命令並實時輸出
process = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)

for line in process.stdout:
    print(line, end="")  # 實時印出每行進度

process.wait()

if process.returncode == 0:
    print(f"\n{output_file} 已完成壓縮。")
else:
    print(f"\n壓縮失敗，錯誤碼: {process.returncode}")
