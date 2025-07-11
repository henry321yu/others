import pandas as pd
import matplotlib.pyplot as plt
from datetime import datetime

# 讀取資料
file_path = r"C:\Users\sgrc - 325\Desktop\seismograph data\perm_0711_91.txt"
df = pd.read_csv(file_path, header=None, names=["timee","time", "ax", "ay", "az", "magnitude", "events", "status", "f", "file"])
markersizee = 0.1

# 將時間欄位轉為 datetime 格式
df["time"] = pd.to_datetime(df["time"], format="%Y-%m-%d %H:%M:%S.%f")

# 繪圖
plt.figure(figsize=(12, 6))
plt.plot(df["timee"], df["ax"], label="ax", marker='.', lw = 1, markersize = markersizee)
plt.plot(df["timee"], df["ay"], label="ay", marker='.', lw = 1, markersize = markersizee)
plt.plot(df["timee"], df["az"], label="az", marker='.', lw = 1, markersize = markersizee)
plt.plot(df["timee"], df["magnitude"], label="magnitude", lw = 1, marker='.', markersize = markersizee)

plt.title(f"{file_path}")
plt.xlabel("Time")
plt.ylabel("Values")
plt.grid(True)
plt.legend()
plt.tight_layout()
plt.xticks(rotation=45)
plt.show()