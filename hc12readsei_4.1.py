import serial
import matplotlib.pyplot as plt
from matplotlib.animation import FuncAnimation
from collections import deque
from math import sqrt
from threading import Thread
from datetime import datetime

import os
import configparser

# ini 檔案名稱
ini_file = "config.ini"

# 建立 ConfigParser 物件
config = configparser.ConfigParser()

# 如果 ini 檔不存在，建立預設設定
if not os.path.exists(ini_file):
    config['SEIS_CONFIGURE'] = {
        'seis_port': 'COM6',
        'baud_rate': '115200',
        'dataL': '1000',
        'k': '0.01',
        'figsize_w': '8',
        'figsize_h': '7'
    }
    with open(ini_file, 'w') as configfile:
        config.write(configfile)
else:
    config.read(ini_file)

# 讀取設定值並套用
port = config['SEIS_CONFIGURE'].get('seis_port', 'COM6')
baudrate = config['SEIS_CONFIGURE'].getint('baud_rate', 115200)
dataL = config['SEIS_CONFIGURE'].getint('dataL', 1000)
k = config['SEIS_CONFIGURE'].getfloat('k', 0.01)
figsize_w = config['SEIS_CONFIGURE'].getfloat('figsize_w', 8)
figsize_h = config['SEIS_CONFIGURE'].getfloat('figsize_h', 6)
figsize = (figsize_w, figsize_h)

start_time = None
last_event_time = None
eventst = 0

# 建立存放資料的 deque，只保留最新 100 筆資料
timee_data = deque(maxlen=dataL)
time_data = deque(maxlen=dataL)
s_data = deque(maxlen=dataL)
x_data = deque(maxlen=dataL)
y_data = deque(maxlen=dataL)
z_data = deque(maxlen=dataL)

# 初始化繪圖（建立 4 個子圖）
fig, axs = plt.subplots(
    4, 1,
    figsize=figsize,
    sharex=True,
    gridspec_kw={'height_ratios': [1, 1, 1, 2.5]}  # s_data 高度為其他的 2 倍
)
(line_x,) = axs[0].plot([], [], label="ax", lw=1)
(line_y,) = axs[1].plot([], [], label="ay", color='orange', lw=1)
(line_z,) = axs[2].plot([], [], label="az", color='green', lw=1)
(line_s,) = axs[3].plot([], [], label="bia", color='red', lw=1)

# 設定子圖標題與 Y 標籤
axs[0].set_ylabel("ax (g)")
axs[1].set_ylabel("ay (g)")
axs[2].set_ylabel("az (g)")
axs[3].set_ylabel("bia (g)")
axs[3].set_xlabel("Program time (s)")

for ax in axs:
    ax.grid(True)
    # ax.legend(loc="upper right")

def update_plot(frame):
    global last_event_time, events, status, f, file, atemp
    """更新繪圖資料"""
    if timee_data and s_data:
        # 更新每條線的資料
        line_x.set_data(timee_data, x_data)
        line_y.set_data(timee_data, y_data)
        line_z.set_data(timee_data, z_data)
        line_s.set_data(timee_data, s_data)

        # 動態調整 Y 軸範圍
        axs[0].set_ylim(min(x_data) - k, max(x_data) + k)
        axs[1].set_ylim(min(y_data) - k, max(y_data) + k)
        axs[2].set_ylim(min(z_data) - k, max(z_data) + k)
        axs[3].set_ylim(min(s_data) - k, max(s_data) + k)

        # 調整 X 軸範圍
        axs[3].set_xlim(min(timee_data), max(timee_data))

        # 更新標題
        axs[0].set_title(f"time: {timee_data[-1]} , {time_data[-1]} , freq: {f} , {atemp}°C\nevents: {events} , status: {status} , last event: {last_event_time}\n{file}" if last_event_time else f"time: {timee_data[-1]} , {time_data[-1]} , freq: {f} , {atemp}°C\nevents: {events} , status: {status}\n{file}")

        axs[0].set_xlabel(f"ax: {x_data[-1]:.5f}")
        axs[1].set_xlabel(f"ay: {y_data[-1]:.5f}")
        axs[2].set_xlabel(f"az: {z_data[-1]:.5f}")
        axs[3].set_xlabel(f"bia: {s_data[-1]:.5f}")

    return line_s, line_x, line_y, line_z

def read_serial_data():
    global start_time, eventst,events, status, f, file,last_event_time, atemp
    """讀取序列埠資料並更新 deque"""
    while True:
        try:
            try:
                # 讀取一行資料並解碼
                line = ser.readline().decode('utf-8', errors='ignore').strip()
            except UnicodeDecodeError as e:
                print(f"解碼錯誤: {e}")
                continue  # 跳過這筆資料
            if line:  # 確保資料不為空
                data = line.split(',')  # 以 "," 分割資料
                # 檢查資料是否有正確的 14 個數據
                if len(data) == 11:
                    try:
                        # 將各個資料轉為浮點數
                        timee = float(data[0])

                        time_value = datetime.strptime(data[1], "%Y-%m-%d %H:%M:%S.%f")

                        if start_time is None:
                            start_time = time_value  # 初始化起始時間

                        ax = float(data[2])
                        ay = float(data[3])
                        az = float(data[4])
                        magnitude = float(data[5])
                        atemp = float(data[6])
                        events = int(data[7])
                        status = data[8]
                        f = float(data[9])
                        file = data[10]

                        if(events != eventst):
                            last_event_time = time_value
                            eventst = events
                            if(last_event_time == start_time):
                                last_event_time = None
                    
                        #設定要繪圖的3軸
                        x = ax
                        y = ay
                        z = az
                        
                        # 計算向量長度 s（若需要）
                        # s = sqrt(x**2 + y**2 + z**2)
                        s = magnitude

                        # 將時間和 x, y, z,s 存入 deque
                        timee_data.append(timee)
                        time_data.append(time_value.strftime("%Y-%m-%d %H:%M:%S.%f")[:-3])
                        s_data.append(s)
                        x_data.append(x)
                        y_data.append(y)
                        z_data.append(z)

                        # 印出資料
                        print(line)
                    except ValueError:
                        print(f"資料轉換失敗: {line}")
                else:
                    print(f"接收到錯誤的資料格式: {line}")
        except Exception as e:
            print(f"發生未知錯誤: {e}")

try:
    # 開啟序列埠
    ser = serial.Serial(port, baudrate, timeout=1)
    print(f"已連接到 {port}，波特率為 {baudrate}")

    # 啟動繪圖動畫
    ani = FuncAnimation(fig, update_plot, interval=100)

    # 啟動資料讀取執行緒
    data_thread = Thread(target=read_serial_data, daemon=True)
    data_thread.start()

    # 顯示繪圖
    plt.show()

except serial.SerialException as e:
    print(f"序列埠錯誤: {e}")
except KeyboardInterrupt:
    print("程式終止")
finally:
    if ser.is_open:
        ser.close()
        print("已關閉序列埠")
