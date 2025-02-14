import serial
import time
import numpy as np
import csv

# 設定序列埠
my_serial = serial.Serial('COM7', 6000000, timeout=1)

# 記錄程式開始時間
start_time = time.perf_counter()
t0 = start_time

# 建立 CSV 檔案
csv_filename = f"lidar_f_{time.strftime('%Y%m%d_%H%M%S')}.csv"

with open(csv_filename, mode="a", newline="") as file:
    writer = csv.writer(file)
    writer.writerow(["time","陣列長度","行數","頻率","t 0","t end","程式讀取時間","data時間差","data 頻率"])

while True:
    # 讀取串口資料
    if my_serial.in_waiting > 0:
        try:
            # 重置計時器
            start_time = time.perf_counter()
            
            read = my_serial.read(2000000).decode().strip()
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
                print(f"{t:.3f}, 陣列長度: {len(read)}, 行數: {count_9999}, 頻率: {F:.2f} Hz")

            # **處理數據，將 `9999` 作為換行符**
            formatted_data = []
            temp_line = []

            for value in arr:
                temp_line.append(value)
                if value == 9999:
                    formatted_data.append(temp_line)
                    temp_line = []

            tt=formatted_data[-1][0]-formatted_data[0][0]
            testn=round(count_9999/2)
            ttt=formatted_data[testn][0]
            tf=count_9999/tt

            print("")
            print(f"t 0: {formatted_data[0][0]:.3f}")
            print(f"t {count_9999}: {formatted_data[-1][0]:.3f}")
            print("")
            print(f"programe t: {elapsed_time:.3f}")
            print(f"data t: {tt:.3f}")
            print(f"tf: {tf:.3f}")
            
            # **將數據寫入 CSV**
            with open(csv_filename, mode="a", newline="") as file:
                writer = csv.writer(file)
                writer.writerow([f"{t:.3f}", len(arr), count_9999, f"{F:.2f}",f"{formatted_data[0][0]:.3f}",f"{formatted_data[-1][0]:.3f}",f"{elapsed_time:.3f}",f"{tt:.3f}",f"{tf:.3f}"])
        except:
            continue