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

# 等 READY
while True:
    line = read_line()
    if line == "READY":
        print("Teensy ready")
        break

# 送 START
ser.write(b"START\n")

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
        print("Size:", remaining_bytes)

        # 讀 binary
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
        print("All files transferred")
        break