import socket
import threading
import math
from collections import deque

from PyQt5 import QtWidgets, QtCore
import pyqtgraph as pg


# =============================
# UDP 設定
# =============================

LOCAL_IP = "0.0.0.0"
UDP_PORT = 2370

sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sock.bind((LOCAL_IP, UDP_PORT))

print(f"Listening ADXL355 UDP port {UDP_PORT}...")


# =============================
# 資料 buffer
# =============================

MAX_POINTS = 30000

ax_buf = deque(maxlen=MAX_POINTS)
ay_buf = deque(maxlen=MAX_POINTS)
az_buf = deque(maxlen=MAX_POINTS)
vector_buf = deque(maxlen=MAX_POINTS)


# =============================
# UDP 接收 thread
# =============================

def udp_receiver():

    while True:
        try:
            data, addr = sock.recvfrom(1024)
            message = data.decode("utf-8")

            parts = message.split(',')

            if len(parts) == 5:
                ID = parts[0]

                ax = float(parts[1])
                ay = float(parts[2])
                az = float(parts[3])
                tem = float(parts[4])

                vector = math.sqrt(ax*ax + ay*ay + az*az)

                ax_buf.append(ax)
                ay_buf.append(ay)
                az_buf.append(az)
                vector_buf.append(vector)

        except:
            pass


# =============================
# GUI
# =============================

class PlotWindow(QtWidgets.QMainWindow):

    def __init__(self):

        super().__init__()

        self.setWindowTitle("ADXL355 Real-time Plot")
        self.resize(900, 800)

        # graphics layout
        self.layout = pg.GraphicsLayoutWidget()
        self.setCentralWidget(self.layout)

        # subplot 1 : vector
        self.plot_vector = self.layout.addPlot(title="Vector Magnitude")
        self.plot_vector.showGrid(x=True, y=True)
        self.curve_vector = self.plot_vector.plot(pen=pg.mkPen('y', width=1))

        self.layout.nextRow()

        # subplot 2 : ax
        self.plot_ax = self.layout.addPlot(title="ax")
        self.plot_ax.showGrid(x=True, y=True)
        self.curve_ax = self.plot_ax.plot(pen=pg.mkPen('r', width=1))

        self.layout.nextRow()

        # subplot 3 : ay
        self.plot_ay = self.layout.addPlot(title="ay")
        self.plot_ay.showGrid(x=True, y=True)
        self.curve_ay = self.plot_ay.plot(pen=pg.mkPen('g', width=1))

        self.layout.nextRow()

        # subplot 4 : az
        self.plot_az = self.layout.addPlot(title="az")
        self.plot_az.showGrid(x=True, y=True)
        self.curve_az = self.plot_az.plot(pen=pg.mkPen('b', width=1))


        # timer
        self.timer = QtCore.QTimer()
        self.timer.timeout.connect(self.update_plot)
        self.timer.start(20)   # 50 Hz GUI refresh


    def update_plot(self):

        if len(ax_buf) == 0:
            return

        # ===== 先做 snapshot (避免thread race) =====
        ax = list(ax_buf)
        ay = list(ay_buf)
        az = list(az_buf)
        vector = list(vector_buf)

        n = min(len(ax), len(ay), len(az), len(vector))

        ax = ax[:n]
        ay = ay[:n]
        az = az[:n]
        vector = vector[:n]

        x = list(range(n))

        # ===== update plot =====
        self.curve_vector.setData(x, vector)
        self.curve_ax.setData(x, ax)
        self.curve_ay.setData(x, ay)
        self.curve_az.setData(x, az)

# =============================
# main
# =============================

def main():

    threading.Thread(target=udp_receiver, daemon=True).start()

    app = QtWidgets.QApplication([])

    win = PlotWindow()
    win.show()

    app.exec_()


if __name__ == "__main__":
    main()