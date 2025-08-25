import subprocess
from pathlib import Path
from datetime import datetime

# 取得今天日期，格式：日月年，例如 250825
today_str = datetime.now().strftime("%d%m%y")

# 壓縮來源資料夾
source_folder = Path(r"C:\Users\sgrc - 325\Desktop\seismograph data\old")
# 輸出壓縮檔路徑
output_file = Path(fr"C:\Users\sgrc - 325\Desktop\seismograph data\old_{today_str}.7z")

# 7z 壓縮命令
# -t7z: 7z 格式
# -mx=7: 壓縮層級 7
# -m0=LZMA2: 壓縮方式 LZMA2
# -md=512m: 字典大小 512MB
# -mfb=273: 字組大小 273
# -mmt=8: CPU 執行緒 8
# 將 "7z" 改成完整路徑
cmd = [
    r"C:\Program Files\7-Zip\7z.exe", "a", str(output_file), str(source_folder),
    "-t7z",
    "-mx=7",
    "-m0=LZMA2",
    "-md=512m",
    "-mfb=273",
    "-mmt=8"
]

# 執行命令
subprocess.run(cmd, check=True)
print(f"{output_file} 已完成壓縮。")
