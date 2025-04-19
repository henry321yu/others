import open3d as o3d
import numpy as np

# 設定方格的範圍和步長
grid_size = 10  # 方格的範圍
step = 1        # 點的間距

# 創建一個包含點的網格
points = []
for x in np.arange(0, grid_size, step):
    for y in np.arange(0, grid_size, step):
        for z in np.arange(0, grid_size, step):
            points.append([x, y, z])

# 將點轉換為 NumPy 陣列
points = np.array(points)

# 創建一個點雲物件
point_cloud = o3d.geometry.PointCloud()
point_cloud.points = o3d.utility.Vector3dVector(points)

# 可選：設置點的顏色
colors = np.zeros_like(points)  # 設定顏色為白色
point_cloud.colors = o3d.utility.Vector3dVector(colors)

# 繪製 3D 點雲
o3d.visualization.draw_geometries([point_cloud])
