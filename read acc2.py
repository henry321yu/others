import pandas as pd
import matplotlib.pyplot as plt
import glob
import os

# 資料夾路徑
folder_path = r"C:\Users\sgrc - 325\Desktop\seismograph data\events"

# 搜尋符合格式的檔案
file_pattern = os.path.join(folder_path, "temp_*.txt")
file_list = sorted(glob.glob(file_pattern))  # 排序

print(f"找到 {len(file_list)} 個檔案")

markersizee = 0.1

for file_path in file_list:
    # 讀取資料
    df = pd.read_csv(
        file_path,
        header=None,
        names=["timee","time", "ax", "ay", "az", "magnitude", "atemp", "events", "status", "f", "file"]
    )

    # 建立新圖表
    fig, ax = plt.subplots(figsize=(12, 6))
    ax.plot(df["timee"], df["ax"], label="ax", lw=1, marker='.', markersize=markersizee)
    ax.plot(df["timee"], df["ay"], label="ay", lw=1, marker='.', markersize=markersizee)
    ax.plot(df["timee"], df["az"]-1, label="az", lw=1, marker='.', markersize=markersizee)
    ax.plot(df["timee"], df["magnitude"], label="magnitude", lw=1, marker='.', markersize=markersizee)

    # 設定標題、標籤
    fig.suptitle(os.path.basename(file_path))  # 視窗標題 & 圖表標題
    ax.set_xlabel("Timee")
    ax.set_ylabel("ax values")
    ax.grid(True)
    ax.legend()

# 一次顯示所有圖
plt.show()
