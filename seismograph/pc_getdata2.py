import serial
import os

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

print("Waiting READY...")

while True:
    line = read_line()
    if line == "READY":
        print("Teensy ready")
        break

# ===== 取得檔案列表 =====
ser.write(b"START\n")

file_list = []

while True:
    line = read_line()

    if line.startswith("FILE_COUNT:"):
        count = int(line.split(":")[1])
        print(f"\nTotal files: {count}\n")

    elif line.startswith("FILE:"):
        name_size = line.replace("FILE:", "")
        name, size = name_size.split(",")

        size = int(size)
        file_list.append((name, size))

        print(f"{name} ({size} bytes)")

    elif line == "END_LIST":
        print("\nFile list received\n")
        break

# ===== 這裡可以做「跳過已存在檔案」=====

# 暫時先全部傳
ser.write(b"CONFIRM\n")

current_file = None
remaining_bytes = 0

while True:
    line = read_line()

    if line.startswith("FILE:"):
        filename = line.replace("FILE:", "")
        filepath = os.path.join(OUTPUT_DIR, filename)
        current_file = open(filepath, "wb")
        print("Receiving:", filename)

    elif line.startswith("SIZE:"):
        remaining_bytes = int(line.replace("SIZE:", ""))

        while remaining_bytes > 0:
            chunk = ser.read(min(BUFFER_SIZE, remaining_bytes))
            if not chunk:
                continue

            current_file.write(chunk)
            remaining_bytes -= len(chunk)

        current_file.close()
        current_file = None
        print("Saved")

    elif line == "DONE":
        print("Transfer complete")
        break