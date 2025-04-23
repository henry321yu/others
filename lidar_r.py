import pandas as pd
import numpy as np
import open3d as o3d

# === 畫格線的函數 ===
def draw_grid_lines(vis, grid_size=1, extent=15):
    lines = []
    points = []
    for i in range(-extent, extent + 1):
        points.append([i * grid_size, -extent * grid_size, 0])
        points.append([i * grid_size, extent * grid_size, 0])
        lines.append([len(points) - 2, len(points) - 1])
        points.append([-extent * grid_size, i * grid_size, 0])
        points.append([extent * grid_size, i * grid_size, 0])
        lines.append([len(points) - 2, len(points) - 1])
    line_set = o3d.geometry.LineSet()
    line_set.points = o3d.utility.Vector3dVector(points)
    line_set.lines = o3d.utility.Vector2iVector(lines)
    line_set.colors = o3d.utility.Vector3dVector([[0.7, 0.7, 0.7]] * len(lines))
    vis.add_geometry(line_set)

# 讀取資料
# file_path = r"C:\Users\sgrc-325\Desktop\git\lidar\lslidarr\lidar_output.txt"
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
points = np.vstack((X, Y, Z)).T

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
colors = np.zeros((len(norm_intensity), 3), dtype=np.float32)

for i, val in enumerate(norm_intensity):
    if val < 0.25:
        t = val * 4
        colors[i] = [0.0, t, 1.0]
    elif val < 0.5:
        t = (val - 0.25) * 4
        colors[i] = [t, 1.0, 0.0]
    elif val < 1.0:
        t = (val - 0.5) * 2
        colors[i] = [1.0, 1.0 - t, 0.0]
    else:
        colors[i] = [1.0, 0.0, 0.0]

pcd = o3d.geometry.PointCloud()
pcd.points = o3d.utility.Vector3dVector(points)
pcd.colors = o3d.utility.Vector3dVector(colors)

# 建立視窗與幾何體
vis = o3d.visualization.Visualizer()
vis.create_window("LiDAR Point Cloud with Grid", width=1000, height=800)
vis.add_geometry(pcd)
draw_grid_lines(vis)

# 更新畫面並抓取 view control
vis.poll_events()
vis.update_renderer()
vc = vis.get_view_control()
vc.set_lookat([0, 0, 0])
vc.set_zoom(0.5)  # 你可以微調這個數值來拉近/拉遠

# 設定點大小與背景
render_option = vis.get_render_option()
render_option.point_size = 2.0
# render_option.background_color = np.array([0, 0, 0])

# 執行視窗
vis.run()
vis.destroy_window()
