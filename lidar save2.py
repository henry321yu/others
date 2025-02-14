import serial
import time
import numpy as np
import csv

# 設定序列埠
my_serial = serial.Serial('COM13', 6000000, timeout=1)

# 記錄程式開始時間
start_time = time.perf_counter()
t0 = start_time

# 建立 CSV 檔案
csv_filename = f"lidar_f_{time.strftime('%Y%m%d_%H%M%S')}.csv"
with open(csv_filename, mode="w", newline="") as file:
    writer = csv.writer(file)
    # 寫入 CSV 標題
    writer.writerow(["時間 (s)", "陣列長度", "行數", "頻率 (Hz)"])

while True:
    # 讀取串口資料
    if my_serial.in_waiting > 0:
        try:
            read = my_serial.read(3000000).decode().strip()
            readi = read.split('\t')  # 根據 tab 分隔數值
            arr = np.array(readi).astype('float32')

            # 計算 9999 出現的次數
            count_9999 = np.count_nonzero(arr == 9999)

            # 計算程式開始後的秒數
            elapsed_time = time.perf_counter()
            t = elapsed_time - t0
            elapsed_time = elapsed_time - start_time

            # 計算頻率 F
            if elapsed_time > 0:
                F = count_9999 / elapsed_time  # Hz
                print(f"{t:.3f}, 陣列長度: {len(arr)}, 行數: {count_9999}, 頻率: {F:.2f} Hz")

                # **將數據寫入 CSV**
                with open(csv_filename, mode="a", newline="") as file:
                    writer = csv.writer(file)
                    writer.writerow([f"{t:.3f}", len(arr), count_9999, f"{F:.2f}"])

            # 重置計時器
            start_time = time.perf_counter()
        except:
            continue
