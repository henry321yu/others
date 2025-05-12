import os
import re
import glob
import pandas as pd
import matplotlib.pyplot as plt
from datetime import datetime, timedelta
import numpy as np
import matplotlib.dates as mdates
import time

# 資料夾路徑
# folder = r'C:\Users\弘銘\Desktop\work from home\git\lidar\lslidarr\\'
# folder = r'C:\Users\sgrc-325\Desktop\git\lidar\lslidarr\\'
folder = ''

# 欲繪圖欄位 index
idx = 7
smoothk = 50

# 開啟互動模式
plt.rcParams['font.family'] = 'Microsoft JhengHei' #使中文編碼正確
plt.ion()
fig, ax = plt.subplots()

while True:
    T_all = pd.DataFrame()
    # 找出所有檔案            
    file_list = sorted(glob.glob(os.path.join(folder, 'lslidar_output_*.txt')))     

    # 讀取並合併所有檔案
    for filepath in file_list:
        filename = os.path.basename(filepath)
        match = re.match(r'lslidar_output_(\d{6})_\d{2}\.txt', filename)
        if not match:
            print(f'忽略檔案：{filename}')
            continue

        date_str = match.group(1)  # YYMMDD
        base_date = datetime.strptime(date_str, '%y%m%d').date()

        print(f'讀取：{filename}，起始日期：{base_date}')

        T = pd.read_csv(filepath, sep=None, engine='python')

        if 'time' not in T.columns:
            continue

        # 轉成 datetime.time 格式
        T['time'] = pd.to_datetime(T['time'], format='%H:%M:%S.%f', errors='coerce').dt.time
        T = T.dropna(subset=['time'])

        # 建立完整 datetime 欄位，考慮跨日
        datetimes = []
        current_date = base_date
        last_datetime = None

        for t in T['time']:
            dt = datetime.combine(current_date, t)
            if last_datetime and dt < last_datetime:
                # 時間跳回來，視為跨日
                current_date += timedelta(days=1)
                dt = datetime.combine(current_date, t)
            datetimes.append(dt)
            last_datetime = dt

        T['datetime'] = datetimes
        T_all = pd.concat([T_all, T], ignore_index=True)

    if T_all.empty:
        print('⚠️ 沒有讀取到任何資料，5 秒後重試')
        time.sleep(5)
        continue

    # 欄位列表與要畫的欄位
    plot_vars = [v for v in T_all.columns if v not in ['time', 'program_time']]
    if idx >= len(plot_vars):
        print("⚠️ 欲繪圖欄位索引超出範圍。")
        break

    y = pd.to_numeric(T_all[plot_vars[idx]], errors='coerce')
    y_smooth = y.rolling(smoothk, min_periods=1).mean()
    x = T_all['datetime']
    x_end=x.iloc[-1]
    x_new=x.iloc[0]
    y_end=y_smooth.iloc[-1]

    print(f'資料為 {x_end.strftime("%Y-%m-%d %H:%M:%S")} 到 {x_new.strftime("%Y-%m-%d %H:%M:%S")} 的 {plot_vars[idx]}')
    print(f'目前值為 {y_end:.3f}')

    # 繪圖
    ax.clear()
    ax.plot(x_end, y_end, 'mo')
    ax.text(x_end, y_end, f'{y_end:.2f}', fontsize=9, color='k')
    ax.plot(x, y_smooth, marker='.', linestyle='None', markersize=0.8)
    ax.set_title(f'資料為 {x_end.strftime("%Y-%m-%d %H:%M:%S")} 到 {x_new.strftime("%Y-%m-%d %H:%M:%S")} 的 {plot_vars[idx]}')
    ax.set_xlabel('Time')
    ax.set_ylabel(plot_vars[idx])
    ax.grid(True)
    ax.xaxis.set_major_formatter(mdates.DateFormatter('%H:%M:%S'))
    fig.autofmt_xdate()
    plt.draw()

    print(f"✅ 圖表更新完成：{datetime.now().strftime('%H:%M:%S')}")
    plt.pause(10)  # 每 k 秒更新一次