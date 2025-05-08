import os
import re
import glob
import pandas as pd
import matplotlib.pyplot as plt
import numpy as np
from datetime import datetime, timedelta
from sklearn.linear_model import LinearRegression
import matplotlib.dates as mdates
from sklearn.preprocessing import PolynomialFeatures
from sklearn.pipeline import make_pipeline
from prophet import Prophet
import warnings
warnings.filterwarnings("ignore")  # 忽略 Prophet 的未來警告

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

# 欄位清單
vars = list(T_all.columns)
plot_vars = [v for v in vars if v not in ['time', 'program_time']]

# 選擇要繪圖的欄位
# {'vert_angle','azimuth','distance','ax','ay','az','temperture','points','frequency_khz_','program_frequency',}
idx = 6  # 可以更換
target_var = plot_vars[idx]
smoothk = 50
model_hr = 4 # 模型小時數
future_hr = 8 # 預測的小時數

# 取得最新時間並篩選近 3 小時資料
latest_s = T_all['program_time'].max()
latest_time = T_all['datetime'].iloc[-1]
oldest_time = T_all['datetime'].iloc[0]
T_recent = T_all[T_all['datetime'] >= latest_time - timedelta(hours=model_hr)].copy()

# 平滑處理
data = T_all[target_var].rolling(smoothk, min_periods=1).mean()
y_recent = pd.to_numeric(T_recent[target_var], errors='coerce').rolling(smoothk, min_periods=1).mean()
x_recent_time = T_recent['datetime']

# 線性回歸預測未來三小時趨勢
X = (x_recent_time - x_recent_time.min()).dt.total_seconds().values.reshape(-1, 1)
y = y_recent.values.reshape(-1, 1)

# 移除 NaN
valid = ~np.isnan(y).flatten()
X_valid = X[valid]
y_valid = y[valid]

# --- Linear Regression 保留原樣 ---

if len(X_valid) > 10:
    # 線性模型
    model_linear = LinearRegression()
    model_linear.fit(X_valid, y_valid)
    future_seconds = np.arange(1, future_hr * 3600 + 1) + X_valid.max()
    future_times = x_recent_time.min() + pd.to_timedelta(future_seconds, unit='s')
    future_pred_linear = model_linear.predict(future_seconds.reshape(-1, 1)) 

    # 多項式模型（2 次）
    degree = 2
    model_poly = make_pipeline(PolynomialFeatures(degree), LinearRegression())
    model_poly.fit(X_valid, y_valid)
    future_pred_poly = model_poly.predict(future_seconds.reshape(-1, 1))

    # Prophet 模型
    df_prophet = pd.DataFrame({'ds': x_recent_time[valid], 'y': y_valid.flatten()})
    model_prophet = Prophet()
    model_prophet.fit(df_prophet)
    future_df = model_prophet.make_future_dataframe(periods=future_hr * 3600, freq='S')
    forecast = model_prophet.predict(future_df)
    future_times_prophet = forecast['ds'][-future_hr * 3600:]
    future_pred_prophet = forecast['yhat'][-future_hr * 3600:]
else:
    print("資料太少，無法進行預測")
    future_times = []
    future_pred_linear = []
    future_pred_poly = []
    future_times_prophet = []
    future_pred_prophet = []

# --- 時間文字顯示 ---
lt_str = oldest_time.strftime('%H:%M:%S.%f')[:-4]
ot_str = latest_time.strftime('%H:%M:%S.%f')[:-4]

if len(future_times) > 0:
    t_str = future_times[-1].strftime('%H:%M:%S.%f')[:-4]
    y_val = round(float(future_pred_linear[-1][0]), 3)
else:
    t_str, y_val = '無資料', float('nan')

print(f"資料為 {lt_str} 到 {ot_str}")
print(f"依據最新的 {model_hr} 小時的 {target_var} 預測 {future_hr} 小時後,在 {t_str} 時的預測模型 ")

# --- 繪圖 ---
plt.figure(figsize=(12, 5))
plt.rcParams['font.family'] = 'Microsoft JhengHei'

# 原始資料
plt.plot(T_all['datetime'], data, '.', markersize=1.2, label=f'全數{latest_s}秒資料')

# 近幾小時資料
plt.plot(x_recent_time, y_recent, 'y.', markersize=1.2, label=f'近{model_hr}小時資料')


last_time = future_times[-1]
last_value_linear = future_pred_linear[-1][0]
last_value_poly = future_pred_poly[-1][0]

last_time_prophet = future_times_prophet.iloc[-1]
last_value_prophet = future_pred_prophet.iloc[-1]

# 預測線繪圖
if len(future_times) > 0:
    # 線性預測 
    plt.plot(future_times, future_pred_linear, 'r--', linewidth=1.2, label=f'線性預測')
    plt.plot(last_time, last_value_linear, 'ro')
    plt.text(last_time, last_value_linear, f'{last_value_linear:.2f}', fontsize=9,
             verticalalignment='bottom', horizontalalignment='right', color='red')
    print(f"[Linear] 預測 {last_time.strftime('%H:%M:%S')}, 值為：{last_value_linear:.4f}")

    # 多項式回歸
    plt.plot(last_time, last_value_poly, 'go')
    plt.plot(future_times, future_pred_poly, 'g--', linewidth=1.2, label=f'多項式回歸 (deg=2)')
    plt.text(last_time, last_value_poly, f'{last_value_poly:.2f}', fontsize=9,
             verticalalignment='bottom', horizontalalignment='right', color='green')
    print(f"[Poly] 預測 {last_time.strftime('%H:%M:%S')}, 值為：{last_value_poly:.4f}")

if len(future_times_prophet) > 0:
    # Prophet預測
    plt.plot(future_times_prophet, future_pred_prophet, 'b--', linewidth=1.2, label=f'Prophet預測')
    plt.plot(last_time_prophet, last_value_prophet, 'bo')
    plt.text(last_time_prophet, last_value_prophet, f'{last_value_prophet:.2f}', fontsize=9,
             verticalalignment='bottom', horizontalalignment='right', color='blue')
    print(f"[Prophet] 預測 {last_time_prophet.strftime('%H:%M:%S')}, 值為：{last_value_prophet:.4f}")


plt.title(f'資料為 {lt_str} 到 {ot_str}\n依據最新的 {model_hr} 小時的 {target_var} 預測 {future_hr} 小時後,在 {t_str} 時的預測模型')
time_fmt = mdates.DateFormatter('%H:%M:%S')
plt.gca().xaxis.set_major_formatter(time_fmt)
plt.xlabel(f'Time')
plt.ylabel(target_var)
plt.legend()
plt.grid(True)
plt.tight_layout()
plt.show()
