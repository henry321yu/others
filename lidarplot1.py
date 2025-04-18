import serial
import matplotlib.pyplot as plt
import numpy as np
import time

# 設定 COM 連線
ser = serial.Serial('COM7', 2000000, timeout=1)

# 限制每批資料數量
max_points = 3000
width = 300

# 初始化 Matplotlib
plt.ion()  # 啟用互動模式
fig, ax = plt.subplots()
sc = ax.scatter([], [], s=1, c='blue')  # 初始化散點圖
ax.set_xlim(-width, width) # office square
ax.set_ylim(-width, width) # office square
# ax.set_ylim(-1000, 0)
ax.set_aspect('equal')
ax.set_xlabel("X (cm)")
ax.set_ylabel("Y (cm)")
ax.set_title("Real-time LiDAR Data")

print(f"開始收集 LiDAR 數據，每 {max_points} 筆重繪一次...")

data_list = []  # 存儲數據的列表

while True:
    try:
        raw_data = ser.read(10000).decode(errors='ignore')  # 讀取數據
        parts = raw_data.split("\t9999\t")  # 以 9999 作為換行分隔

        for part in parts:
            fields = part.strip().split("\t")  # 根據制表符分割數據
            if len(fields) != 6:
                continue  # 確保有 6 個欄位

            try:
                timee, angle, distance = map(float, fields[:3])  # 只取前三個欄位
            except ValueError:
                continue  # 如果轉換失敗，跳過這筆數據

            # 轉換為 x, y 座標
            angle_rad = np.radians(90 - angle)
            x = distance * np.cos(angle_rad)
            y = distance * np.sin(angle_rad)

            data_list.append((x, y))  # 加入新數據

            # 當數據達到 max_points 時，更新繪圖並清空舊數據
            if len(data_list) >= max_points:
                current_time = time.strftime("%H:%M:%S") + f".{(time.time() % 1):.2f}"[2:]  # 取得當前時間

                x_vals, y_vals = zip(*data_list)  # 轉換為 x, y 陣列
                sc.set_offsets(np.c_[x_vals, y_vals])  # 更新點座標
                plt.draw()
                plt.pause(0.01)  # 避免 CPU 過載

                # 輸出到終端
                print(f"[{current_time}] 已收集 {max_points} 筆數據，清除舊數據並繪製新數據...")

                # 更新標題
                ax.set_title(f"Real-time LiDAR Data - Last {max_points} Points\n[{current_time}]")

                data_list.clear()  # 清空舊數據，開始收集下一批

    except KeyboardInterrupt:
        print("程序終止")
        break

ser.close()
plt.ioff()
plt.show()