import os
import re
import glob
import pandas as pd
import matplotlib.pyplot as plt
from datetime import datetime, timedelta
import matplotlib.dates as mdates

# 資料夾路徑
# folder = r'C:\Users\弘銘\Desktop\work from home\git\lidar\lslidarr\\'
# folder = r'C:\Users\sgrc-325\Desktop\git\lidar\lslidarr\\'
folder = ''

# 初始化
T_all = pd.DataFrame()

# 找出所有檔案
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

# 檢查是否有資料
if T_all.empty:
    raise ValueError("沒有讀取到任何資料。")

# 排序
T_all = T_all.sort_values('datetime')
x = T_all['datetime']

# 可用欄位
plot_vars = [col for col in T_all.columns if col not in ['time', 'datetime', 'program_time']]

# 範例：單圖繪圖
idx = 6  # 根據實際欄位選擇
smoothk = 50
y = pd.to_numeric(T_all[plot_vars[idx]], errors='coerce')
y_smooth = y.rolling(smoothk, min_periods=1).mean()

plt.plot(x, y_smooth, marker='.', linestyle='None', markersize=0.8)
plt.title(plot_vars[idx])
plt.xlabel("時間")
plt.ylabel(plot_vars[idx])
plt.grid(True)
plt.gca().xaxis.set_major_formatter(mdates.DateFormatter('%m-%d %H:%M:%S'))
plt.xticks(rotation=45)
plt.tight_layout()
plt.show()
