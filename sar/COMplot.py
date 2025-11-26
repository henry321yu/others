from PyQt5 import QtWidgets, QtCore
import pyqtgraph as pg
import serial
from threading import Thread
from collections import deque
from datetime import datetime
import sys

#-----------------------------------------
#  固定參數
#-----------------------------------------
com_port = 'COM6'
baud_rate = 115200

#-----------------------------------------
#  資料緩衝
#-----------------------------------------
data_length = 1000  # 緩衝資料筆數
time_data = deque(maxlen=data_length)
distance_data = deque(maxlen=data_length)

latest_strength = 0
latest_temp = 0

start_time = datetime.now()


#-----------------------------------------
#  主視窗
#-----------------------------------------
class LidarPlot(QtWidgets.QMainWindow):
    def __init__(self):
        super().__init__()

        self.setWindowTitle("Dot LIDAR Distance Monitor")
        self.resize(900, 600)

        # 建立 PlotWidget
        self.plot_widget = pg.PlotWidget()
        self.setCentralWidget(self.plot_widget)

        # 設定座標軸
        self.plot_widget.setBackground('w')
        self.plot_widget.showGrid(x=True, y=True)
        self.plot_widget.setLabel('left', 'Distance', units='m')
        self.plot_widget.setLabel('bottom', 'Time', units='s')

        # 曲線
        self.curve = self.plot_widget.plot(
            pen=pg.mkPen(color='red', width=1)  # <<< 線條顏色
        )

        # 設定更新計時器
        self.timer = QtCore.QTimer()
        self.timer.timeout.connect(self.update_plot)
        self.timer.start(16)  # 每 50ms 更新

    #-----------------------------------------
    #  更新資料/畫面
    #-----------------------------------------
    def update_plot(self):
        if len(time_data) == 0:
            return 

        # 更新曲線
        self.curve.setData(time_data, distance_data)

        # 設定 y 軸範圍
        ymin = min(distance_data) - 0.5
        ymax = max(distance_data) + 0.5
        self.plot_widget.setYRange(ymin, ymax)

        # 更新視窗 title 顯示強度+溫度
        self.setWindowTitle(
            f"LIDAR Distance Monitor   |   Strength: {latest_strength}   |   Temp: {latest_temp}°C"
        )

#-----------------------------------------
#  序列埠讀取執行緒
#-----------------------------------------
def serial_reader():
    global latest_strength, latest_temp

    try:
        ser = serial.Serial(com_port, baud_rate, timeout=1)
        print(f"Connected to {com_port} at {baud_rate}")
    except Exception as e:
        print("Serial port error:", e)
        return

    while True:
        try:
            line = ser.readline().decode("utf-8").strip()
            if not line:
                continue

            parts = line.split(",")

            if len(parts) != 3:
                print("Invalid data:", line)
                continue

            # 解析資料：距離(m), 強度, 溫度
            distance = float(parts[0])
            strength = int(parts[1])
            temp = float(parts[2])
            # if(distance == 0):
            #     continue
            temp = temp + 22

            # 記錄時間（程式啟動後的秒數）
            t = (datetime.now() - start_time).total_seconds()

            time_data.append(t)
            distance_data.append(distance)
            latest_strength = strength
            latest_temp = temp

            print(f"{t:.3f}s | {distance}m | {strength} | {temp}°C")

        except Exception as e:
            print("Error:", e)
            continue


#-----------------------------------------
#  主程式
#-----------------------------------------
if __name__ == "__main__":
    thread = Thread(target=serial_reader, daemon=True)
    thread.start()

    app = QtWidgets.QApplication(sys.argv)
    window = LidarPlot()
    window.show()
    sys.exit(app.exec_())
