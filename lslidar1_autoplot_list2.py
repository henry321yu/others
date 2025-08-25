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
folder = ''  # 設定你的資料夾路徑

# 欲繪圖欄位 index 列表
# {'vert_angle','azimuth','distance','board2_temp','ax','ay','az','temperture','points','frequency_khz_','program_frequency',}
# idx_list = [3, 7, 9, 10]
idx_list = [7, 3]
smoothk = 1

plt.rcParams['font.family'] = 'Microsoft JhengHei'  # 中文編碼正確
plt.ion()
fig, ax = plt.subplots()

while True:
    T_all = pd.DataFrame()
    file_list = sorted(glob.glob(os.path.join(folder, 'lslidar_output_*.txt')))

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

        T['time'] = pd.to_datetime(T['time'], format='%H:%M:%S.%f', errors='coerce').dt.time
        T = T.dropna(subset=['time'])

        datetimes = []
        current_date = base_date
        last_datetime = None

        for t in T['time']:
            dt = datetime.combine(current_date, t)
            if last_datetime and dt < last_datetime:
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

    plot_vars = [v for v in T_all.columns if v not in ['time', 'program_time']]
    if any(idx >= len(plot_vars) for idx in idx_list):
        print("⚠️ 有欲繪圖欄位索引超出範圍。")
        break

    x = T_all['datetime']
    x_1 = x.iloc[-1]
    x_0 = x.iloc[0]
    ymax = 0

    ax.clear()
    for idx in idx_list:
        var_name = plot_vars[idx]
        y = pd.to_numeric(T_all[var_name], errors='coerce')
        y_smooth = y.rolling(smoothk, min_periods=1).mean()
        y_end = y_smooth.iloc[-1]
        if(max(y) > ymax):
            ymax=max(y)

        ax.plot(x_1, y_end, 'o', label=f'{var_name}')
        ax.text(x_1, y_end, f'{y_end:.2f}', fontsize=9)
        ax.plot(x, y_smooth, marker='.', linestyle='None', markersize=0.8, label=var_name)

        print(f'{var_name} 目前值為 {y_end:.3f}')

    # 如果超出 0~400 的範圍才調整 Y 軸
    if ymax > 400:
        ax.set_ylim(min(y) - 10, 400)
    else:
        ax.set_ylim(min(y) - 10, max(y) + 10)

    ax.set_title(f'資料為 {x_0.strftime("%Y-%m-%d %H:%M:%S")} 到 {x_1.strftime("%Y-%m-%d %H:%M:%S")} 的多筆欄位資料')
    ax.set_xlabel('Time')
    ax.set_ylabel('Value')
    ax.grid(True)
    ax.legend()
    ax.xaxis.set_major_formatter(mdates.DateFormatter('%m-%d %H:%M:%S'))
    fig.autofmt_xdate()
    plt.draw()

    print(f"✅ 圖表更新完成：{datetime.now().strftime('%H:%M:%S')}")
    plt.pause(10)
