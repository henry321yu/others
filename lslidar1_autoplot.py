import os
import glob
import pandas as pd
import matplotlib.pyplot as plt
from datetime import datetime
import numpy as np
import matplotlib.dates as mdates
import time

# 資料夾路徑
folder = r'C:\Users\弘銘\Desktop\work from home\git\lidar\lslidarr\\'
# folder = r'C:\Users\sgrc-325\Desktop\git\lidar\lslidarr\\'

# 欲繪圖欄位 index
plot_idx = 6
smoothk = 50

# 開啟互動模式
plt.ion()
fig, ax = plt.subplots()

while True:
    T_all = pd.DataFrame()
    program_time_offset = 0

    # 讀取並合併所有檔案
    for i in range(0, 50):
        filename = os.path.join(folder, f'lslidar_output_{i:02d}.txt')
        if os.path.isfile(filename):            
            print(f'檔案：{filename}')        
            T = pd.read_csv(filename, sep=None, engine='python')
            if 'time' in T.columns:
                T['time'] = T['time'].astype(str)
            if 'program_time' in T.columns:
                T['program_time'] = T['program_time'] + program_time_offset
                program_time_offset = T['program_time'].iloc[-1]
                print(f'更新時間篇移量 {program_time_offset} ')   
            T_all = pd.concat([T_all, T], ignore_index=True)

    if T_all.empty:
        print('⚠️ 沒有讀取到任何資料，5 秒後重試')
        time.sleep(5)
        continue

    # 處理時間欄位
    T_all['time'] = pd.to_datetime(T_all['time'], format='%H:%M:%S.%f', errors='coerce')
    T_all = T_all.dropna(subset=['time'])
    base_time = T_all['time'].iloc[0]
    T_all['time_a'] = base_time + pd.to_timedelta(T_all['program_time'], unit='s')

    # 欄位列表與要畫的欄位
    plot_vars = [v for v in T_all.columns if v not in ['time', 'program_time']]
    if plot_idx >= len(plot_vars):
        print("⚠️ 欲繪圖欄位索引超出範圍。")
        break

    y = pd.to_numeric(T_all[plot_vars[plot_idx]], errors='coerce')
    y_smooth = y.rolling(smoothk, min_periods=1).mean()
    x = T_all['time_a']

    # 繪圖
    ax.clear()
    ax.plot(x, y_smooth, marker='.', linestyle='None', markersize=0.8)
    ax.set_title(plot_vars[plot_idx])
    ax.set_ylabel(plot_vars[plot_idx])
    ax.grid(True)
    ax.xaxis.set_major_formatter(mdates.DateFormatter('%H:%M:%S'))
    fig.autofmt_xdate()
    plt.draw()

    print(f"✅ 圖表更新完成：{datetime.now().strftime('%H:%M:%S')}")
    plt.pause(5)  # 每 k 秒更新一次