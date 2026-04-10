import serial
import os
import time

PORT = 'COM13'
BAUD = 921600

OUTPUT_DIR = r"C:\Users\sgrc - 325\Desktop\seismograph data\arduino_transfer\3b"
os.makedirs(OUTPUT_DIR, exist_ok=True)

ser = serial.Serial(PORT, BAUD, timeout=1)

BUFFER_SIZE = 4096
SIZE_THRESHOLD = 4 * 1024 **3  # 1MB


def format_size(size_bytes):
    if size_bytes is None:
        return "Unknown"

    if size_bytes < 1024:
        return f"{size_bytes:.1f} bytes"
    elif size_bytes < 1024**2:
        return f"{size_bytes/1024:.1f} KB"
    elif size_bytes < 1024**3:
        return f"{size_bytes/1024**2:.1f} MB"
    else:
        return f"{size_bytes/1024**3:.2f} GB"

def clean_text(s):
    return ''.join(ch for ch in s if ord(ch) >= 32).strip()

def read_line():
    line = b""
    while True:
        c = ser.read(1)
        if c == b'\n':
            break
        if c:
            line += c
    return clean_text(line.decode(errors='ignore'))

def is_valid_filename(name):
    try:
        name.encode('ascii')  # 或 utf-8
    except:
        return False

    # 過濾不可見字元
    if any(ord(c) < 32 for c in name):
        return False

    return True

# ===== HELLO (reconnect safe) =====
ser.write(b"HELLO\n")

print("Waiting READY...")

while True:
    line = read_line()
    if line == "READY":
        print("Teensy ready")
        break

# ===== START =====
ser.write(b"START\n")

file_list = []

# ===== Read list =====
while True:
    line = read_line()

    if line.startswith("FILE_COUNT:"):
        print(f"\nTotal files: {line.split(':')[1]}")

    elif line.startswith("TOTAL_SIZE:"):
        total_size = int(line.split(":")[1])
        print(f"Total size: {format_size(total_size)}\n")

    elif line.startswith("FILE:"):
        name_size = line[len("FILE:"):].strip()

        # ===== 基本格式檢查 =====
        if "," not in name_size:
            print(f"[WARN] Corrupted FILE line (no comma): {repr(line)}")
            continue

        parts = name_size.split(",", 1)

        if len(parts) != 2:
            print(f"[WARN] Corrupted FILE line (split error): {repr(line)}")
            continue

        name, size_str = parts

        name = name.strip().split("/")[-1]

        # ===== size 轉換保護 =====
        try:
            size = int(size_str.strip())
        except ValueError:
            print(f"[WARN] Invalid size in FILE line: {repr(line)}")
            continue

        # ===== 過濾奇怪檔名（可選但建議）=====
        if not name or any(ord(c) < 32 for c in name):
            print(f"[WARN] Invalid filename: {repr(name)}")
            continue

        file_list.append((name, size))
        print(f"{name} ({format_size(size)})")

    elif line == "END_LIST":
        print("\nList received\n")
        break

# ===== CONFIRM =====
ser.write(b"CONFIRM\n")

current_file = None
file_start_time = None
total_received = 0
had_timeout = False

while True:
    line = read_line()

    if line.startswith("FILE:"):
        raw_name = line.replace("FILE:", "")
        current_filename = raw_name.split(",")[0].strip().split("/")[-1]

        if (
            not is_valid_filename(current_filename) or
            current_filename == "" or
            # len(current_filename) > 255 or
            # not current_filename.endswith(".TXT")
            len(current_filename) > 255
            # not current_filename.endswith(".TXT")
        ):
            print(f"[AUTO SKIP] {repr(current_filename)}")
            ser.write(b"SKIP\n")
            continue

        filepath = os.path.join(OUTPUT_DIR, current_filename)

        expected_size = None
        skip = False

        # ===== match file =====
        for name, size in file_list:
            if name == current_filename:
                expected_size = size
                if size >= SIZE_THRESHOLD or (os.path.exists(filepath) and os.path.getsize(filepath) == size):
                # if os.path.exists(filepath) and os.path.getsize(filepath) == size:
                    skip = True
                break

        # ===== fallback if not found =====
        if expected_size is None:
            print(f"Warning: unknown file {current_filename}")
            ser.write(b"OK\n")
            current_file = open(filepath, "wb")
            file_start_time = time.time()
            total_received = 0
        else:
            if skip:
                print(f"Skip: {current_filename} ({format_size(expected_size)})")
                ser.write(b"SKIP\n")
                current_file = None
                file_start_time = None
            else:
                ser.write(b"OK\n")
                current_file = open(filepath, "wb")
                file_start_time = time.time()
                total_received = 0
                print(f"Receiving: {current_filename} ({format_size(expected_size)})")

    elif line.startswith("SIZE:"):
        remaining_bytes = int(line.split(":")[1])
        total_received = 0

        last_data_time = time.time()
        while remaining_bytes > 0:
            chunk = ser.read(min(BUFFER_SIZE, remaining_bytes))
            if chunk:
                last_data_time = time.time()

                if current_file:
                    current_file.write(chunk)

                remaining_bytes -= len(chunk)
                total_received += len(chunk)

            else:
                # ⚠️ 超過 timeout → 視為傳輸結束
                if time.time() - last_data_time > 2:
                    print("\n[WARN] Timeout waiting for remaining data")
                    if remaining_bytes != 0:
                        print(f"[WARN] Missing {remaining_bytes} bytes")
                        had_timeout = True
                    break

            if current_file and file_start_time:
                elapsed = time.time() - file_start_time
                if elapsed > 0:
                    speed = total_received / elapsed
                    line = f"Received: {format_size(total_received)}  Speed: {format_size(speed)}/s"
                    print("\r" + line.ljust(80), end="")


        if current_file:
            current_file.close()

            elapsed = time.time() - file_start_time
            speed = (total_received / (1024 * 1024)) / elapsed if elapsed > 0 else 0

            print('\r' + ' ' * (len(line)) + '\r', end='')
            print(f"Saved: {current_filename} ({format_size(total_received)})")

        ser.write(b"ACK\n")
        ser.reset_input_buffer()

    elif line == "DONE":
        print("\nTransfer complete")

        if had_timeout:
            print("[INFO] Timeout occurred in this session. Continuing for verification...")
            had_timeout = False

            # 重啟整個流程（重新 HELLO + START）
            ser.write(b"HELLO\n")

            while True:
                line = read_line()
                if line == "READY":
                    print("Teensy ready again")

                    ser.write(b"START\n")
                    break

            # 清空 buffer 避免殘留
            continue
        else:
            print("[INFO] No timeout detected. Exiting cleanly.")
            time.sleep(5)
            break