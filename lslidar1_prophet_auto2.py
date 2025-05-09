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
import time
import warnings

warnings.filterwarnings("ignore")

# 資料夾路徑
# folder = r'C:\Users\弘銘\Desktop\work from home\git\lidar\lslidarr\\'
# folder = r'C:\Users\sgrc-325\Desktop\git\lidar\lslidarr\\'
folder = ''

def process_and_plot():
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
        return

    vars = list(T_all.columns)
    plot_vars = [v for v in vars if v not in ['time', 'program_time']]
    # 選擇要繪圖的欄位
    idx = 6  # 可以更換
    target_var = plot_vars[idx]
    smoothk = 50
    model_hr = 2 # 模型小時數
    future_hr = 2 # 預測的小時數

    latest_time = T_all['datetime'].iloc[-1]
    oldest_time = T_all['datetime'].iloc[0]
    latest_s = T_all['program_time'].max()
    T_recent = T_all[T_all['datetime'] >= latest_time - timedelta(hours=model_hr)].copy()

    data = T_all[target_var].rolling(smoothk, min_periods=1).mean()
    y_recent = pd.to_numeric(T_recent[target_var], errors='coerce').rolling(smoothk, min_periods=1).mean()
    x_recent_time = T_recent['datetime']

    X = (x_recent_time - x_recent_time.min()).dt.total_seconds().values.reshape(-1, 1)
    y = y_recent.values.reshape(-1, 1)

    valid = ~np.isnan(y).flatten()
    X_valid = X[valid]
    y_valid = y[valid]

    future_times, future_pred_linear = [], []
    future_pred_poly, future_times_prophet, future_pred_prophet = [], [], []

    if len(X_valid) > 10:
        model_linear = LinearRegression().fit(X_valid, y_valid)
        future_seconds = np.arange(1, future_hr * 3600 + 1) + X_valid.max()
        future_times = x_recent_time.min() + pd.to_timedelta(future_seconds, unit='s')
        future_pred_linear = model_linear.predict(future_seconds.reshape(-1, 1))

        model_poly = make_pipeline(PolynomialFeatures(2), LinearRegression())
        model_poly.fit(X_valid, y_valid)
        future_pred_poly = model_poly.predict(future_seconds.reshape(-1, 1))

        df_prophet = pd.DataFrame({'ds': x_recent_time[valid], 'y': y_valid.flatten()})
        model_prophet = Prophet()
        model_prophet.fit(df_prophet)
        future_df = model_prophet.make_future_dataframe(periods=future_hr * 3600, freq='S')
        forecast = model_prophet.predict(future_df)
        future_times_prophet = forecast['ds'][-future_hr * 3600:]
        future_pred_prophet = forecast['yhat'][-future_hr * 3600:]

    plt.clf()
    plt.rcParams['font.family'] = 'Microsoft JhengHei' #使中文編碼正確

    x_end=T_all['datetime'].iloc[-1]
    y_end=data.iloc[-1]
    plt.plot(x_end, y_end, 'mo')
    plt.text(x_end, y_end, f'{y_end:.2f}', fontsize=9, color='k')

    plt.plot(T_all['datetime'], data, '.', markersize=1.2, label=f'全數{latest_s:.2f}秒資料')
    plt.plot(x_recent_time, y_recent, 'y.', markersize=1.2, label=f'近{model_hr}小時資料')

    if len(future_times) > 0:
        last_time = future_times[-1]
        last_value_linear = future_pred_linear[-1][0]
        last_value_poly = future_pred_poly[-1][0]

        plt.plot(future_times, future_pred_linear, 'r--', linewidth=1.2, label='線性預測')
        plt.plot(last_time, last_value_linear, 'ro')
        plt.text(last_time, last_value_linear, f'{last_value_linear:.2f}', fontsize=9, color='red')

        plt.plot(future_times, future_pred_poly, 'g--', linewidth=1.2, label='多項式回歸 (deg=2)')
        plt.plot(last_time, last_value_poly, 'go')
        plt.text(last_time, last_value_poly, f'{last_value_poly:.2f}', fontsize=9, color='green')

    if len(future_times_prophet) > 0:
        last_time_prophet = future_times_prophet.iloc[-1]
        last_value_prophet = future_pred_prophet.iloc[-1]
        plt.plot(future_times_prophet, future_pred_prophet, 'b--', linewidth=1.2, label='Prophet預測')
        plt.plot(last_time_prophet, last_value_prophet, 'bo')
        plt.text(last_time_prophet, last_value_prophet, f'{last_value_prophet:.2f}', fontsize=9, color='blue')


    f_t_str = future_times[-1].strftime('%H:%M:%S.%f')[:-4]
    print(f'資料為 {oldest_time.strftime("%Y-%m-%d %H:%M:%S")} 到 {latest_time.strftime("%Y-%m-%d %H:%M:%S")} 的 {target_var}\n依據最新{model_hr} 小時的資料,預測 {future_hr} 小時後,在 {f_t_str} 時的預測模型')
    print(f'目前值為 {y_end:.3f}')
    print(f"[Linear] 預測 {last_time.strftime('%H:%M:%S')}, 值為：{last_value_linear:.3f}")
    print(f"[Poly] 預測 {last_time.strftime('%H:%M:%S')}, 值為：{last_value_poly:.3f}")
    print(f"[Prophet] 預測 {last_time_prophet.strftime('%H:%M:%S')}, 值為：{last_value_prophet:.3f}")

    plt.title(f'資料為 {oldest_time.strftime("%Y-%m-%d %H:%M:%S")} 到 {latest_time.strftime("%Y-%m-%d %H:%M:%S")} 的 {target_var}\n依據最新{model_hr} 小時的資料,預測 {future_hr} 小時後,在 {f_t_str} 時的預測模型')
    plt.xlabel('Time')
    plt.ylabel(plot_vars[idx])
    plt.gca().xaxis.set_major_formatter(mdates.DateFormatter('%H:%M:%S'))
    plt.xticks(rotation=45)
    plt.legend()
    plt.grid(True)
    plt.tight_layout()
    plt.draw()
    plt.pause(30) # 你來 delay

# 主迴圈：每 60 秒更新一次
plt.ion()
while True:
    print(f"\n--- 更新時間：{datetime.now().strftime('%H:%M:%S')} ---")
    process_and_plot()
    # time.sleep(30)  # 這裡可以開啟 sleep，延遲 30 秒再更新
