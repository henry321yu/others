import serial
import os
import time

PORT = 'COM12'
BAUD = 921600

OUTPUT_DIR = r"C:\Users\sgrc - 325\Desktop\seismograph data\tran_sd"
os.makedirs(OUTPUT_DIR, exist_ok=True)

ser = serial.Serial(PORT, BAUD, timeout=1)

BUFFER_SIZE = 4096

def read_line():
    line = b""
    while True:
        c = ser.read(1)
        if c == b'\n':
            break
        if c:
            line += c
    return line.decode(errors='ignore').strip()

def format_size(size_bytes):
    """自動將大小轉換為 B/KB/MB/GB"""
    if size_bytes < 1024:
        return f"{size_bytes:.1f} bytes"
    elif size_bytes < 1024**2:
        return f"{size_bytes/1024:.1f} KB"
    elif size_bytes < 1024**3:
        return f"{size_bytes/1024**2:.1f} MB"
    else:
        return f"{size_bytes/1024**3:.2f} GB"

print("Waiting READY...")

# 等 READY
while True:
    line = read_line()
    if line == "READY":
        print("Teensy ready")
        break

# 取得檔案列表
ser.write(b"START\n")

file_list = []
total_size = 0

while True:
    line = read_line()

    if line.startswith("FILE_COUNT:"):
        count = int(line.split(":")[1])
        print(f"\nTotal files: {count}")

    elif line.startswith("TOTAL_SIZE:"):
        total_size = int(line.split(":")[1])
        print(f"Total size: {format_size(total_size)}\n")

    elif line.startswith("FILE:"):
        name_size = line.replace("FILE:", "")
        name, size = name_size.split(",")
        size = int(size)

        file_list.append((name, size))
        print(f"{name} ({format_size(size)})")

    elif line == "END_LIST":
        print("\nList received\n")
        break

# ===== 跳過已存在檔案 =====
files_to_transfer = []

for name, size in file_list:
    local_path = os.path.join(OUTPUT_DIR, name)

    if os.path.exists(local_path):
        if os.path.getsize(local_path) == size:
            print(f"Skip: {name}")
            continue

    files_to_transfer.append(name)

# ===== 開始傳輸 =====
ser.write(b"CONFIRM\n")

current_file = None
remaining_bytes = 0
current_filename = None
file_start_time = 0

while True:
    line = read_line()

    if line.startswith("FILE:"):
        current_filename = line.replace("FILE:", "")
        filepath = os.path.join(OUTPUT_DIR, current_filename)

        # 檢查是否 skip
        skip = False
        expected_size = None

        # 從已知 list 找 size
        for name, size in file_list:
            if name == current_filename:
                expected_size = size
                if os.path.exists(filepath) and os.path.getsize(filepath) == size:
                    skip = True
                break

        if skip:
            print(f"Skip: {current_filename} ({format_size(expected_size)})")
            current_file = None
            remaining_bytes = 0
        else:
            current_file = open(filepath, "wb")
            file_start_time = time.time()
            print(f"Receiving: {current_filename} ({format_size(expected_size) if expected_size else ''})")

    elif line.startswith("SIZE:"):
        remaining_bytes = int(line.replace("SIZE:", ""))

        total_received = 0

        while remaining_bytes > 0:
            chunk = ser.read(min(BUFFER_SIZE, remaining_bytes))
            if not chunk:
                continue

            if current_file:
                current_file.write(chunk)

            remaining_bytes -= len(chunk)
            total_received += len(chunk)

            # 顯示速度（每秒更新一次）
            elapsed = time.time() - file_start_time
            if elapsed > 0:
                speed = (total_received / (1024 )) / elapsed
                # print(f"\rSpeed: {speed:.2f} KB/s", end="")

        if current_file:
            current_file.close()

            elapsed = time.time() - file_start_time
            size_mb = total_received / (1024 * 1024)
            speed = size_mb / elapsed if elapsed > 0 else 0

            print(f"Saved: {current_filename} ({format_size(total_received)}) @ {speed:.2f} MB/s")

        current_file = None
        remaining_bytes = 0

    elif line == "DONE":
        print("\nTransfer complete")
        break