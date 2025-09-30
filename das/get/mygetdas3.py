import ctypes
import numpy as np
import matplotlib.pyplot as plt
import time

# --------- DLL 設定 ---------
daq = ctypes.CDLL("./DasCardDemodLib.dll")
daq.DasCardOpen.restype = ctypes.c_int
daq.DasCardClose.restype = None
daq.DasCardStart.restype = ctypes.c_int
daq.DasCardStop.restype = ctypes.c_int
daq.DasCardSetResolution.argtypes = [ctypes.c_int]
daq.DasCardSetSampleNum.argtypes = [ctypes.c_int]
daq.DasCardSetPulseWidth.argtypes = [ctypes.c_int]
daq.DasCardSetPulseFrq.argtypes = [ctypes.c_int]
daq.DasCardSetPulseNum.argtypes = [ctypes.c_int]
daq.DasCardSetDataSel.argtypes = [ctypes.c_int]
daq.DasCardQueryFifo.restype = ctypes.c_int
daq.DasCardReadFifo.argtypes = [ctypes.c_char_p, ctypes.c_int]
daq.DasCardReadFifo.restype = ctypes.c_int

# --------- 打開卡片 ---------
assert daq.DasCardOpen() == 0, "無法打開 GY-DAQ-2480"
daq.DasCardSetResolution(1)      # 1=0.4m
daq.DasCardSetSampleNum(768)     # 必須為256倍數
daq.DasCardSetPulseWidth(50)     # ns
daq.DasCardSetPulseFrq(1000)     # Hz
daq.DasCardSetPulseNum(1)        # 每次快取段數
daq.DasCardSetDataSel(2)         # 2 = Ch1 Amplitude+Phase
daq.DasCardStart()

# --------- 初始化參數 ---------
resolution = 0.4
size = daq.DasCardQueryFifo()
buf = ctypes.create_string_buffer(size)

x_start = 300
x_end = 550
num_x = x_end - x_start
time_window = 200  # 顯示 200 秒

# 創建空白瀑布圖 buffer
waterfall = np.zeros((time_window, num_x), dtype=np.uint16)

# 初始化 matplotlib
plt.ion()
fig, ax = plt.subplots(figsize=(12, 6))
im = ax.imshow(waterfall, aspect='auto', origin='lower',
               extent=[x_start*resolution, x_end*resolution, 0, time_window],
               cmap='jet', vmin=1.5, vmax=4.5)  # vmax 可調整顏色強度

ax.set_xlabel("Position (m)")
ax.set_ylabel("Time (s)")
ax.set_title("DAS Live Waterfall")
plt.colorbar(im, ax=ax, label="Amplitude")

t = 0
try:
    while True:
        ret = daq.DasCardReadFifo(buf, size)
        if ret == 1:
            data = np.frombuffer(buf.raw, dtype=np.int16)
            amplitude = data[0::2].astype(np.uint16)

            # 取 300~550 的資料點
            line = amplitude[x_start:x_end]

            # 滑動瀑布圖 buffer
            waterfall[:-1] = waterfall[1:]  # 上移一行
            waterfall[-1] = line  # 最新一筆放在最後一行

            # 更新圖片
            im.set_data(waterfall)
            ax.set_ylim(max(0, t - time_window), t)  # Y軸顯示最近 200 秒
            fig.canvas.draw()
            fig.canvas.flush_events()

            t += 1
        else:
            time.sleep(0.01)

except KeyboardInterrupt:
    print("手動停止")

# 停止
daq.DasCardStop()
daq.DasCardClose()
plt.ioff()
plt.show()
