import serial
import matplotlib.pyplot as plt
from matplotlib.animation import FuncAnimation
from collections import deque
from math import sqrt
from threading import Thread
from datetime import datetime


# 設定序列埠
port = "COM3"
baudrate = 115200  # 根據實際情況調整
dataL = 500 # 設定要繪圖的資料數
k = 0.01 #y軸留白大小
start_time = None
last_event_time = None
eventst = 0

# 建立存放資料的 deque，只保留最新 100 筆資料
timee = deque(maxlen=dataL)
time_data = deque(maxlen=dataL)
s_data = deque(maxlen=dataL)
x_data = deque(maxlen=dataL)
y_data = deque(maxlen=dataL)
z_data = deque(maxlen=dataL)

# 初始化繪圖（建立 4 個子圖）
fig, ax = plt.subplots()
line_s, = ax.plot([], [], label="bia", lw=1)
ax.set_title("Real-time Plot")
ax.set_xlabel("time")
ax.set_ylabel("Value")
ax.grid(True)
ax.legend()

def update_plot(frame):
    global last_event_time, events, status, f, file
    """更新繪圖資料"""
    if timee and s_data:
        # 更新每條線的資料
        line_s.set_data(timee, s_data)

        # 動態調整 Y 軸範圍
        ax.set_ylim(min(s_data) - k, max(s_data) + k)

        # 調整 X 軸範圍
        ax.set_xlim(min(timee), max(timee))

        # 更新標題
        ax.set_title(f"time: {timee[-1]} , {time_data[-1]} , last event: {last_event_time}\nevents: {events} , status: {status} , freq: {f}" if last_event_time else f"time: {timee[-1]} , {time_data[-1]}\nevents: {events} , status: {status} , freq: {f}")

        ax.set_xlabel(f"bia: {s_data[-1]:.5f}")

    return line_s

def read_serial_data():
    global start_time , eventst
    """讀取序列埠資料並更新 deque"""
    while True:
        # 讀取一行資料並解碼
        line = ser.readline().decode('utf-8').strip()
        if line:  # 確保資料不為空
            data = line.split(',')  # 以 "," 分割資料
            # 檢查資料是否有正確的 14 個數據
            if len(data) >= 7:
                try:
                    # 將各個資料轉為浮點數
                    time_value = datetime.strptime(data[0], "%Y-%m-%d %H:%M:%S.%f")

                    if start_time is None:
                        start_time = time_value  # 初始化起始時間
                    elapsed_seconds = (time_value - start_time).total_seconds()

                    global events, status, f, file
                    ax = float(data[1])
                    ay = float(data[2])
                    az = float(data[3])
                    magnitude = float(data[4])
                    events = int(data[5])
                    status = data[6]
                    f = float(data[7])
                    # file = data[8]

                    global last_event_time
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
                    timee.append(elapsed_seconds)
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
