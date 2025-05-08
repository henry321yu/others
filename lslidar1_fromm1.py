import os
import glob
import pandas as pd
import matplotlib.pyplot as plt
from datetime import datetime
import numpy as np
import matplotlib.dates as mdates

# 資料夾路徑
# folder = r'C:\Users\弘銘\Desktop\work from home\git\lidar\lslidarr\\'
folder = r'C:\Users\sgrc-325\Desktop\git\lidar\lslidarr\\'

# 初始化表格
T_all = pd.DataFrame()
program_time_offset = 0

# 讀取並合併所有檔案
for i in range(0, 50):
    filename = os.path.join(folder, f'lslidar_output_{i:02d}.txt')
    if os.path.isfile(filename):
        print(f'檔案：{filename}')        
        T = pd.read_csv(filename, sep=None, engine='python')  # 自動偵測欄位 # 自動分隔符號        
        if 'time' in T.columns: # 確保 'time' 欄位為字串（若存在）
            T['time'] = T['time'].astype(str)
        if 'program_time' in T.columns:
            T['program_time'] = T['program_time'] + program_time_offset            
            program_time_offset = T['program_time'].iloc[-1] # 更新偏移量為目前最後一筆時間（累加） 
            print(f'更新時間篇移量 {program_time_offset} ')   
        T_all = pd.concat([T_all, T], ignore_index=True) # 合併到總表格

# 確認有資料
if T_all.empty:
    raise ValueError('沒有讀取到任何資料。')

# x 軸：time# 處理時間欄位
T_all['time'] = pd.to_datetime(T_all['time'], format='%H:%M:%S.%f', errors='coerce')
T_all = T_all.dropna(subset=['time'])
base_time = T_all['time'][0]
T_all['time_a'] = base_time + pd.to_timedelta(T_all['program_time'], unit='s')
x = T_all['time_a']

# 將 time 轉為 datetime 類型（如果需要的話）
x = pd.to_datetime(x, format='%H:%M:%S.%f', errors='coerce')  # 可調整格式以符合原資料

# 所有欄位
vars = list(T_all.columns)

# 要畫的欄位（排除 time 與 program_time）
plot_vars = [v for v in vars if v not in ['time', 'program_time']]

# # 建立多張圖，每張圖畫一個變數對 time
# for idx, var in enumerate(plot_vars):
#     plt.figure(idx)
#     y = pd.to_numeric(T_all[var], errors='coerce')
#     smoothk = 50
#     y_smooth = y.rolling(smoothk, min_periods=1).mean()
#     plt.plot(x, y_smooth, marker='.', linestyle='None', markersize=0.8) # 以時間排列
#     # plt.plot(y_smooth, marker='.', markersize=0.8)  # 以檔案排列
#     plt.title(var)
#     plt.ylabel(var)
#     plt.grid(True)
# plt.show()

# 印出指定 plot
# 例如: {'vert_angle','azimuth','distance','ax','ay','az','temperture','points','frequency_khz_','program_frequency',}
idx = 6
smoothk = 50
y = pd.to_numeric(T_all[plot_vars[idx]], errors='coerce')
y_smooth = y.rolling(smoothk, min_periods=1).mean()
plt.plot(x, y_smooth, marker='.', linestyle='None', markersize=0.8) # 以時間排列
# plt.plot(y_smooth, marker='.', markersize=0.8)  # 以檔案排列

plt.title(plot_vars[idx])
time_fmt = mdates.DateFormatter('%H:%M:%S')
plt.gca().xaxis.set_major_formatter(time_fmt)
plt.ylabel(plot_vars[idx])
plt.grid(True)
plt.show()