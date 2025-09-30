import ctypes
import numpy as np
import matplotlib.pyplot as plt
import time

# 載入 DLL
daq = ctypes.CDLL("./DasCardDemodLib.dll")

# 基本函式定義
daq.DasCardOpen.restype = ctypes.c_int
daq.DasCardClose.restype = None
daq.DasCardStart.restype = ctypes.c_int
daq.DasCardStop.restype = ctypes.c_int

# 設定函式
daq.DasCardSetResolution.argtypes = [ctypes.c_int]
daq.DasCardSetSampleNum.argtypes = [ctypes.c_int]
daq.DasCardSetPulseWidth.argtypes = [ctypes.c_int]
daq.DasCardSetPulseFrq.argtypes = [ctypes.c_int]
daq.DasCardSetPulseNum.argtypes = [ctypes.c_int]
daq.DasCardSetDataSel.argtypes = [ctypes.c_int]

# FIFO 讀取
daq.DasCardQueryFifo.restype = ctypes.c_int
daq.DasCardReadFifo.argtypes = [ctypes.c_char_p, ctypes.c_int]
daq.DasCardReadFifo.restype = ctypes.c_int

# 1. 開啟卡
assert daq.DasCardOpen() == 0, "無法打開 GY-DAQ-2480"

# 2. 參數設定
daq.DasCardSetResolution(1)      # 1=0.4m
daq.DasCardSetSampleNum(768)     # 必須為256倍數
daq.DasCardSetPulseWidth(50)     # ns
daq.DasCardSetPulseFrq(1000)     # Hz
daq.DasCardSetPulseNum(1)        # 每次快取段數
daq.DasCardSetDataSel(2)         # 2 = Ch1 Amplitude+Phase

# 3. 開始采集
daq.DasCardStart()

# 解析度
resolution = 0.4  # m
size = daq.DasCardQueryFifo()
buf = ctypes.create_string_buffer(size)

# 初始化畫圖
plt.ion()
fig, ax = plt.subplots(figsize=(10, 6))
line, = ax.plot([], [], label="Amplitude")
ax.set_xlim(0, 768 * resolution)   # 固定 X 軸範圍
ax.set_ylim(0, 65535)              # uint16 最大值
ax.set_xlabel("Fiber Length (m)")
ax.set_ylabel("Amplitude")
ax.set_title("Distributed Acoustic Sensing Amplitude Profile (Live)")
ax.legend()

try:
    while True:
        ret = daq.DasCardReadFifo(buf, size)
        if ret == 1:
            data = np.frombuffer(buf.raw, dtype=np.int16)
            amplitude = data[0::2].astype(np.uint16)

            # 每次只顯示最新的一筆 (0~768)
            distance = np.arange(len(amplitude)) * resolution
            line.set_data(distance, amplitude)

            # 自動調整 Y 軸
            ax.set_ylim(0, max(100, np.max(amplitude) * 1.1))

            fig.canvas.draw()
            fig.canvas.flush_events()
        else:
            time.sleep(0.01)

except KeyboardInterrupt:
    print("手動停止")

# 5. 停止
daq.DasCardStop()
daq.DasCardClose()
plt.ioff()
plt.show()
