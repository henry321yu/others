import ctypes
import numpy as np
import matplotlib.pyplot as plt

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
daq.DasCardSetResolution(1)      # 1=0.4m, 2=0.8m, ...
daq.DasCardSetSampleNum(768)  # 取樣點數 (需為256倍數)
daq.DasCardSetPulseWidth(50)    # ns
daq.DasCardSetPulseFrq(1000)     # Hz
daq.DasCardSetPulseNum(1)       # 每次快取段數
daq.DasCardSetDataSel(2)         # 2 = Ch1 Amplitude+Phase

# 3. 開始采集
daq.DasCardStart()

# 4. 等待數據
size = daq.DasCardQueryFifo()
buf = ctypes.create_string_buffer(size)
ret = daq.DasCardReadFifo(buf, size)

if ret == 1:
    data = np.frombuffer(buf.raw, dtype=np.int16)  # 相位為 short，振幅為 ushort
    amplitude = data[0::2].astype(np.uint16)
    phase = data[1::2].astype(np.int16) / 256.0  # 轉換成 rad

    # 計算對應距離軸
    resolution = 0.4  # m, 依 DasCardSetResolution 設定
    distance = np.arange(len(amplitude)) * resolution

    # 畫圖
    plt.figure(figsize=(10,6))
    plt.plot(distance, amplitude, label="Amplitude")
    plt.xlabel("Fiber Length (m)")
    plt.ylabel("Amplitude")
    plt.title("Distributed Acoustic Sensing Amplitude Profile")
    plt.legend()
    plt.show()

# 5. 停止
daq.DasCardStop()
daq.DasCardClose()
