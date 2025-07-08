import pandas as pd
import matplotlib.pyplot as plt
from datetime import datetime

# 讀取資料
file_path = r"C:\Users\sgrc - 325\Desktop\seismograph data\perm_0707_79.txt"
df = pd.read_csv(file_path, header=None, names=["time", "ax", "ay", "az", "magnitude"])
markersizee = 0.1

# 將時間欄位轉為 datetime 格式
df["time"] = pd.to_datetime(df["time"], format="%Y-%m-%d %H:%M:%S.%f")

# 繪圖
plt.figure(figsize=(12, 6))
plt.plot(df["time"], df["ax"], label="ax", marker='.', markersize = markersizee)
plt.plot(df["time"], df["ay"], label="ay", marker='.', markersize = markersizee)
plt.plot(df["time"], df["az"], label="az", marker='.', markersize = markersizee)
plt.plot(df["time"], df["magnitude"], label="magnitude", marker='.', markersize = markersizee)

plt.title("Sensor Data Over Time")
plt.xlabel("Time")
plt.ylabel("Values")
plt.grid(True)
plt.legend()
plt.tight_layout()
plt.xticks(rotation=45)
plt.show()