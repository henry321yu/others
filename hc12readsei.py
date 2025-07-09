import serial
import matplotlib.pyplot as plt
from matplotlib.animation import FuncAnimation
from collections import deque
from math import sqrt
from threading import Thread
from datetime import datetime


# 設定序列埠
port = "COM6"
baudrate = 115200  # 根據實際情況調整
dataL = 500 # 設定要繪圖的資料數
k=5 #y軸留白大小
start_time = None  # 新增這行

# 建立存放資料的 deque，只保留最新 100 筆資料
timee = deque(maxlen=dataL)
time_data = deque(maxlen=dataL)
s_data = deque(maxlen=dataL)
x_data = deque(maxlen=dataL)
y_data = deque(maxlen=dataL)
z_data = deque(maxlen=dataL)
events_data = deque(maxlen=dataL)
status_data = deque(maxlen=dataL)

# 初始化繪圖
fig, ax = plt.subplots()
line_s, = ax.plot([], [], label="magnitude", lw=1)
line_x, = ax.plot([], [], label="ax", lw=1)
line_y, = ax.plot([], [], label="ay", lw=1)
line_z, = ax.plot([], [], label="az", lw=1)
ax.set_title("Real-time Plot")
ax.set_xlabel("time")
ax.set_ylabel("Value")
ax.grid(True)
ax.legend()

def update_plot(frame):
    """更新繪圖資料"""
    if time_data and s_data:
        # 更新線條資料
        line_s.set_data(timee, s_data)
        # line_x.set_data(timee, x_data)
        # line_y.set_data(timee, y_data)
        # line_z.set_data(timee, z_data)
        
        # 動態調整 x 軸和 y 軸範圍
        ax.set_xlim(min(timee), max(timee))
        ax.set_ylim(min(s_data)-k, max(s_data)+k)
        # ax.set_ylim(min(min(x_data), min(y_data), min(z_data)) - k, 
        #             max(max(x_data), max(y_data), max(z_data)) + k)
        
        # 更新軸標籤
        ax.set_xlabel(f"program time: {timee[-1]} , {time_data[-1]}")
        ax.set_title(f"ax: {x_data[-1]:.5f} , ay: {y_data[-1]:.5f} , az: {z_data[-1]:.5f}\nmagnitude: {s_data[-1]:.5f} , events: {events_data[-1]:.0f}, status: {status_data[-1]}" if time_data else "")
        
    return line_x, line_y, line_z, line_s

def read_serial_data():
    global start_time
    """讀取序列埠資料並更新 deque"""
    while True:
        # 讀取一行資料並解碼
        line = ser.readline().decode('utf-8').strip()
        if line:  # 確保資料不為空
            data = line.split(',')  # 以 "," 分割資料
            # 檢查資料是否有正確的 14 個數據
            if len(data) == 7:
                try:
                    # 將各個資料轉為浮點數
                    time_value = datetime.strptime(data[0], "%Y-%m-%d %H:%M:%S.%f")

                    if start_time is None:
                        start_time = time_value  # 初始化起始時間
                    elapsed_seconds = (time_value - start_time).total_seconds()

                    ax = float(data[1])
                    ay = float(data[2])
                    az = float(data[3])
                    magnitude = float(data[4])
                    events = float(data[5])
                    status = data[6]
                    
                    global k

                    #設定要繪圖的3軸
                    
                    x=ax 
                    y=ay
                    z=az
                    k=0.01 #y軸留白大小
                    
                    # 計算向量長度 s（若需要）
                    # s = sqrt(x**2 + y**2 + z**2)
                    s = magnitude

                    # 將時間和 x, y, z,s 存入 deque
                    timee.append(elapsed_seconds)
                    time_data.append(time_value)
                    s_data.append(s)
                    x_data.append(x)
                    y_data.append(y)
                    z_data.append(z)
                    events_data.append(events)
                    status_data.append(status)

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
