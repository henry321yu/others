from PyQt5 import QtWidgets, QtCore
import pyqtgraph as pg
from threading import Thread
from collections import deque
from datetime import datetime
import socket
import sys

#-----------------------------------------
# UDP 監聽設定
#-----------------------------------------
LISTEN_IP = "0.0.0.0"  # 監聽所有網卡
LISTEN_PORT = 5300
BUFFER_SIZE = 1024

#-----------------------------------------
# 資料緩衝
#-----------------------------------------
data_length = 1000
time_data = deque(maxlen=data_length)
distance_data = deque(maxlen=data_length)

latest_strength = 0
latest_temp = 0

start_time = datetime.now()

#-----------------------------------------
# PyQt5 GUI
#-----------------------------------------
class LidarPlot(QtWidgets.QMainWindow):
    def __init__(self):
        super().__init__()

        self.setWindowTitle("UDP LIDAR Distance Monitor")
        self.resize(900, 600)

        self.plot_widget = pg.PlotWidget()
        self.setCentralWidget(self.plot_widget)

        self.plot_widget.setBackground('w')
        self.plot_widget.showGrid(x=True, y=True)
        self.plot_widget.setLabel('left', 'Distance', units='m')
        self.plot_widget.setLabel('bottom', 'Time', units='s')

        # 曲線顏色可自訂
        self.curve = self.plot_widget.plot(
            pen=pg.mkPen(color='red', width=1)
        )

        # 計時器更新
        self.timer = QtCore.QTimer()
        self.timer.timeout.connect(self.update_plot)
        self.timer.start(16)  # 20 FPS

    def update_plot(self):
        if len(time_data) == 0:
            return

        self.curve.setData(time_data, distance_data)

        # 動態 Y 軸
        ymin = min(distance_data) - 0.5
        ymax = max(distance_data) + 0.5
        self.plot_widget.setYRange(ymin, ymax)

        # Title 顯示最新強度 + 溫度
        self.setWindowTitle(
            f"UDP LIDAR Monitor   |   Strength: {latest_strength}   |   Temp: {latest_temp}°C"
        )

#-----------------------------------------
# UDP 資料接收執行緒
#-----------------------------------------
def udp_reader():
    global latest_strength, latest_temp

    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.bind((LISTEN_IP, LISTEN_PORT))
    print(f"UDP Server listening on {LISTEN_IP}:{LISTEN_PORT}")

    while True:
        try:
            data, addr = sock.recvfrom(BUFFER_SIZE)
            line = data.decode().strip()
            if not line:
                continue

            parts = line.split(",")
            if len(parts) != 3:
                print("Invalid data:", line)
                continue

            # 解析距離, 強度, 溫度
            distance = float(parts[0])
            strength = int(parts[1])
            temp = float(parts[2])
            # if(distance == 0):
            #     continue
            temp = temp + 22

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
# 主程式
#-----------------------------------------
if __name__ == "__main__":
    # 啟動 UDP 讀取執行緒
    thread = Thread(target=udp_reader, daemon=True)
    thread.start()

    app = QtWidgets.QApplication(sys.argv)
    window = LidarPlot()
    window.show()
    sys.exit(app.exec_())
