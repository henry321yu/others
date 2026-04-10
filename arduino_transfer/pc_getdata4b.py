import serial
import os
import time

PORT = 'COM13'
BAUD = 921600

OUTPUT_DIR = r"C:\Users\sgrc - 325\Desktop\seismograph data\arduino_transfer\3b"
os.makedirs(OUTPUT_DIR, exist_ok=True)

ser = serial.Serial(PORT, BAUD, timeout=1)

BUFFER_SIZE = 4096
SIZE_THRESHOLD = 4 * 1024 **3  # 4GB


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
        name.encode('ascii')
    except:
        return False

    if any(ord(c) < 32 for c in name):
        return False

    return True


# ✅ NEW：重新讀 LIST（關鍵修正）
def read_file_list():
    file_list = []

    while True:
        line = read_line()

        if line.startswith("FILE_COUNT:"):
            print(f"\nTotal files: {line.split(':')[1]}")

        elif line.startswith("TOTAL_SIZE:"):
            total_size = int(line.split(":")[1])
            print(f"Total size: {format_size(total_size)}\n")

        elif line.startswith("FILE:"):
            name_size = line[len("FILE:"):].strip()

            if "," not in name_size:
                continue

            name, size_str = name_size.split(",", 1)

            name = name.strip().split("/")[-1]

            try:
                size = int(size_str.strip())
            except:
                continue

            if not name or any(ord(c) < 32 for c in name):
                continue

            file_list.append((name, size))
            print(f"{name} ({format_size(size)})")

        elif line == "END_LIST":
            print("\nList received\n")
            break

    return file_list


# ===== HELLO =====
ser.reset_input_buffer()
ser.reset_output_buffer()
ser.write(b"HELLO\n")

print("Waiting READY...")

while True:
    line = read_line()
    if line == "READY":
        print("Teensy ready")
        break

# ===== START =====
ser.write(b"START\n")

file_list = read_file_list()

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
            len(current_filename) > 255
        ):
            print(f"[AUTO SKIP] {repr(current_filename)}")
            ser.write(b"SKIP\n")
            continue

        filepath = os.path.join(OUTPUT_DIR, current_filename)

        expected_size = None
        skip = False

        for name, size in file_list:
            if name == current_filename:
                expected_size = size
                if size >= SIZE_THRESHOLD or (os.path.exists(filepath) and os.path.getsize(filepath) == size):
                    skip = True
                break

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

    if current_file is not None and current_file.closed:
        current_file = None
        
    elif line.startswith("SIZE:"):
        remaining_bytes = int(line.split(":")[1])
        total_received = 0

        last_data_time = time.time()

        while remaining_bytes > 0:
            chunk = ser.read(min(BUFFER_SIZE, remaining_bytes))

            if chunk:
                last_data_time = time.time()

                if current_file is not None:
                    try:
                        current_file.write(chunk)
                    except ValueError:
                        print("[ERROR] Attempt to write to closed file (ignored)")
                        current_file = None

                remaining_bytes -= len(chunk)
                total_received += len(chunk)

            else:
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
                    line2 = f"Received: {format_size(total_received)}  Speed: {format_size(speed)}/s"
                    # print("\r" + line2.ljust(80), end="")

        if current_file:
            current_file.close()
            current_file = None

            # print('\r' + ' ' * 80 + '\r', end='')
            print(f"Saved: {current_filename} ({format_size(total_received)})")

        ser.write(b"ACK\n")
        ser.reset_input_buffer()

    elif line == "DONE":
        print("\nTransfer complete")

        if had_timeout:
            print("[INFO] Timeout occurred. Restarting session...\n")
            had_timeout = False

            # ✅ 關鍵修正：完整重跑流程
            ser.reset_input_buffer()
            ser.reset_output_buffer()

            ser.write(b"HELLO\n")

            while True:
                line = read_line()
                if line == "READY":
                    print("Teensy ready again")
                    ser.write(b"START\n")
                    break

            # ✅ 重新讀 LIST（關鍵）
            file_list = read_file_list()

            # ✅ 再 CONFIRM
            ser.write(b"CONFIRM\n")

            continue
        else:
            print("[INFO] No timeout detected. Exiting cleanly.")
            time.sleep(3)
            break