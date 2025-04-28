import pandas as pd
import numpy as np
import matplotlib.pyplot as plt
from mpl_toolkits.mplot3d import Axes3D

# 讀取資料
file_path = "lidar_output.txt"
data = pd.read_csv(file_path)

# 將角度轉成弧度
azimuth = np.deg2rad(data["azimuth"])
vert_angle = np.deg2rad(data["vert_angle"])
distance = data["distance"]
intensity = data["intensity"]

# 球座標轉直角座標
X = distance * np.cos(vert_angle) * np.sin(azimuth)
Y = distance * np.cos(vert_angle) * np.cos(azimuth)
Z = distance * np.sin(vert_angle)

# 頻率統計
t = data["time"]
duration = t.iloc[-1]
freq_kHz = len(t) / duration / 1000
print(f"time length: {duration:.3f} s")
print(f"data length: {len(t)/1e6} M points")
print(f"f: {freq_kHz:.2f} kHz")

# 顏色對應（根據 intensity）
intensity = np.array(intensity, dtype=np.float32)
norm_intensity = np.clip(intensity / 255.0, 0, 1)
colors = []

for val in norm_intensity:
    if val < 0.25:
        t = val * 4
        colors.append([0.0, t, 1.0])
    elif val < 0.5:
        t = (val - 0.25) * 4
        colors.append([t, 1.0, 0.0])
    elif val < 1.0:
        t = (val - 0.5) * 2
        colors.append([1.0, 1.0 - t, 0.0])
    else:
        colors.append([1.0, 0.0, 0.0])

colors = np.array(colors)

# 建立 3D 圖
fig = plt.figure(figsize=(10, 8))
ax = fig.add_subplot(111, projection='3d')

# 只保留最後 k 筆點雲資料
k = 64000
X = X[-k:]
Y = Y[-k:]
Z = Z[-k:]
colors = colors[-k:]

# 顯示點雲
ax.scatter(X, Y, Z, c=colors, s=1)

# 計算資料中心與最大範圍
x_range = X.max() - X.min()
y_range = Y.max() - Y.min()
z_range = Z.max() - Z.min()
max_range = max(x_range, y_range, z_range)

x_center = (X.max() + X.min()) / 2
y_center = (Y.max() + Y.min()) / 2
z_center = (Z.max() + Z.min()) / 2

# 設定軸的範圍為相同大小
ax.set_xlim(x_center - max_range/2, x_center + max_range/2)
ax.set_ylim(y_center - max_range/2, y_center + max_range/2)
ax.set_zlim(z_center - max_range/2, z_center + max_range/2)

# 設定視角與標籤
ax.set_xlabel('X')
ax.set_ylabel('Y')
ax.set_zlabel('Z')
ax.set_title('LiDAR Point Cloud with Grid')
ax.view_init(elev=90, azim=-90)  # 從Z往-Z看
ax.set_box_aspect([1,1,1])  # 調整比例以避免壓縮變形

plt.tight_layout()
plt.show()
