import socket
import struct
import numpy as np
import sys
from PyQt5 import QtWidgets, QtCore
import pyqtgraph as pg
import matplotlib.cm as cm

# ===== DAS Server 設定 =====
SERVER_IP = "192.168.137.10"
SERVER_PORT = 6789

FRAME_HEADER = 0xa55a5aa5
FRAME_TRAILER = 0xaa5555aa

SAMPLE_POINTS = 5120
RESOLUTION = 0.4
PULSE_FREQ = 8000

DATA_POINTS = int(SAMPLE_POINTS / (RESOLUTION / 0.1) * (PULSE_FREQ / 20))
BYTES_PER_POINT = 4
FRAME_DATA_SIZE = DATA_POINTS * BYTES_PER_POINT

# ===== 顯示設定 =====
DISPLAY_LEN = int(SAMPLE_POINTS / (RESOLUTION / 0.1))
NUM_FRAMES = 300
UPDATE_HZ = 100
UPDATE_INTERVAL_MS = int(1000 / UPDATE_HZ)

# ===== 降採樣與時間平滑參數 =====
skipN = 1      # 空間降採樣，每隔 skipN 個 sample 取一次
smoothY = 2    # 時間平滑 (幀方向滑動平均)
smoothX = 3    # 空間平滑 (X方向)，同 smoothY 方式做滑動平均

DISPLAY_LEN_SKIP = DISPLAY_LEN // skipN

# ===== TCP 工具函數 =====
def find_header(sock):
    buffer = b''
    while True:
        chunk = sock.recv(4096)
        if not chunk:
            return None, None
        buffer += chunk
        for i in range(len(buffer) - 3):
            val_le = struct.unpack('<I', buffer[i:i+4])[0]
            if val_le == FRAME_HEADER:
                return buffer[i+4:], '<'
        buffer = buffer[-3:]

def recv_all(sock, length):
    data = b''
    while len(data) < length:
        packet = sock.recv(length - len(data))
        if not packet:
            return None
        data += packet
    return data

# ===== 主 GUI 類別 =====
class DASWaterfall(QtWidgets.QMainWindow):
    def __init__(self):
        super().__init__()
        self.setWindowTitle("DAS Waterfall (MATLAB Equivalent + Smooth + Skip)")
        self.resize(1000, 600)

        # 建立 PyQtGraph 畫布
        self.plot_widget = pg.PlotWidget()
        self.setCentralWidget(self.plot_widget)
        self.img_item = pg.ImageItem()
        self.plot_widget.addItem(self.img_item)

        self.plot_widget.setLabel('left', 'Frame (n)')
        self.plot_widget.setLabel('bottom', 'Sampling Points')
        self.plot_widget.setTitle('DAS Waterfall (MATLAB Equivalent)')

        # 使用 jet colormap
        jet_lut = (cm.get_cmap('gray')(np.linspace(0, 1, 256)) * 255).astype(np.uint8)
        self.img_item.setLookupTable(jet_lut)

        # 初始化 buffer (frames, samples)
        self.phase_buffer = np.zeros((NUM_FRAMES, DISPLAY_LEN_SKIP), dtype=np.float32)

        # 啟動資料接收執行緒
        self.thread = DataThread()
        self.thread.data_signal.connect(self.update_frame)
        self.thread.start()

        # 更新定時器
        self.timer = QtCore.QTimer()
        self.timer.timeout.connect(self.refresh_plot)
        self.timer.start(UPDATE_INTERVAL_MS)

    def update_frame(self, frame_data):
        """更新單幀資料，支持 dataSkip"""
        # 1️⃣ 資料平方 (MATLAB 等價)
        power = frame_data ** 2

        # 2️⃣ 空間降採樣
        power_skip = power[:DISPLAY_LEN:skipN]
        
        # 3️⃣ 空間平滑 (X方向 smoothX)
        if smoothX > 1:
            kernel = np.ones(smoothX) / smoothX
            power_skip = np.convolve(power_skip, kernel, mode='same')

        # 4️⃣ 滾動 buffer
        self.phase_buffer = np.roll(self.phase_buffer, -1, axis=0)
        self.phase_buffer[-1, :len(power_skip)] = power_skip

    def refresh_plot(self):
        """更新瀑布圖，時間方向平滑"""
        smoothed_buffer = np.copy(self.phase_buffer)
        if smoothY > 1:
            kernel = np.ones(smoothY) / smoothY
            for i in range(smoothed_buffer.shape[1]):
                smoothed_buffer[:, i] = np.convolve(self.phase_buffer[:, i], kernel, mode='same')

        self.img_item.setImage(smoothed_buffer, autoLevels=False)
        self.img_item.setLevels([0, 0.01])
        self.plot_widget.repaint()

# ===== 資料接收線程 =====
class DataThread(QtCore.QThread):
    data_signal = QtCore.pyqtSignal(np.ndarray)

    def run(self):
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.connect((SERVER_IP, SERVER_PORT))
        print(f"已連線到 DAS Server: {SERVER_IP}:{SERVER_PORT}")

        buffer, endian = find_header(sock)
        if buffer is None:
            print("找不到 header，退出")
            return

        print(f"使用位元組序: {'little-endian' if endian == '<' else 'big-endian'}")

        try:
            while True:
                data_bytes = buffer
                if len(data_bytes) < FRAME_DATA_SIZE:
                    rest = recv_all(sock, FRAME_DATA_SIZE - len(data_bytes))
                    if rest is None:
                        break
                    data_bytes += rest

                trailer_bytes = recv_all(sock, 4)
                if trailer_bytes is None:
                    break

                trailer = struct.unpack(f'{endian}I', trailer_bytes)[0]
                if trailer != FRAME_TRAILER:
                    print("Trailer 不符，重新同步中...")
                    buffer, endian = find_header(sock)
                    continue

                # === 讀取 float32 ===
                data = np.frombuffer(data_bytes[:FRAME_DATA_SIZE], dtype=np.float32)

                # 取前 DISPLAY_LEN samples
                frame_display = data[:DISPLAY_LEN]

                # 丟到 GUI
                self.data_signal.emit(frame_display)

                # 尋找下一個 header
                buffer, endian = find_header(sock)
                if buffer is None:
                    break

        finally:
            sock.close()
            print("連線已關閉")

# ===== 主程式 =====
if __name__ == "__main__":
    app = QtWidgets.QApplication(sys.argv)
    window = DASWaterfall()
    window.show()
    sys.exit(app.exec_())
