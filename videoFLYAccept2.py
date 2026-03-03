import socket
import threading
import matplotlib.pyplot as plt
from datetime import datetime
import time
import os
import cv2
import numpy as np

# ========== 使用者設定 ==========
ACCPORT_PIXEL = 2386
ACCPORT_IMAGE = 2385
BUFFER_SIZE = 65536
SAVE_INTERVAL = 30      # 每 30 秒儲存一次 pixel 資料
MAX_POINTS = 300        # 圖上最多顯示多少個點
FRAMES_PER_VIDEO = 500 # 每部影片 2000 幅影像
OUTPUT_FOLDER = "pixel_logs"
VIDEO_FOLDER = "video_logs"
os.makedirs(OUTPUT_FOLDER, exist_ok=True)
os.makedirs(VIDEO_FOLDER, exist_ok=True)
# ===============================

pixel_data = []  # [(timestamp, avg20, avg30, avg40, avg50)]
data_lock = threading.Lock()
latest_frame = None
frame_lock = threading.Lock()

video_writer = None
video_frame_count = 0
video_start_time = None
video_lock = threading.Lock()

def pixel_receiver():
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.bind(("0.0.0.0", ACCPORT_PIXEL))
    print(f"📩 Pixel 接收中 (port {ACCPORT_PIXEL})...")

    while True:
        try:
            data, addr = sock.recvfrom(BUFFER_SIZE)
            message = data.decode('utf-8')
            parts = message.strip().split(',')
            if len(parts) == 5:
                timestamp = parts[0]
                avg_vals = list(map(int, parts[1:]))
                with data_lock:
                    pixel_data.append((timestamp, *avg_vals))
                    if len(pixel_data) > MAX_POINTS:
                        pixel_data.pop(0)
            else:
                print(f"⚠️ 格式錯誤: {message}")
        except Exception as e:
            print(f"⚠️ Pixel 接收錯誤: {e}")

def image_receiver():
    global latest_frame, video_writer, video_frame_count, video_start_time
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.bind(("0.0.0.0", ACCPORT_IMAGE))
    print(f"🖼️ 影像接收中 (port {ACCPORT_IMAGE})...")

    while True:
        try:
            data, addr = sock.recvfrom(BUFFER_SIZE)
            np_data = np.frombuffer(data, dtype=np.uint8)
            frame = cv2.imdecode(np_data, cv2.IMREAD_COLOR)
            if frame is not None:
                with frame_lock:
                    latest_frame = frame.copy()

                # ===== 儲存成影片邏輯 =====
                with video_lock:
                    if video_writer is None:
                        video_start_time = datetime.now().strftime("%Y%m%d_%H%M%S")
                        video_filename = os.path.join(VIDEO_FOLDER, f"video_{video_start_time}.avi")
                        height, width = frame.shape[:2]
                        fourcc = cv2.VideoWriter_fourcc(*'XVID')
                        video_writer = cv2.VideoWriter(video_filename, fourcc, 2, (width, height))
                        print(f"📹 開始錄製影片: {video_filename}")

                    video_writer.write(frame)
                    video_frame_count += 1

                    if video_frame_count >= FRAMES_PER_VIDEO:
                        video_writer.release()
                        print(f"✅ 影片儲存完成，共 {video_frame_count} 幅")
                        video_writer = None
                        video_frame_count = 0

        except Exception as e:
            print(f"⚠️ 影像接收錯誤: {e}")

def pixel_saver():
    while True:
        time.sleep(SAVE_INTERVAL)
        filename = datetime.now().strftime("pixel_%Y%m%d_%H%M%S.csv")
        filepath = os.path.join(OUTPUT_FOLDER, filename)
        with data_lock:
            with open(filepath, "w") as f:
                f.write("timestamp,avg20,avg30,avg40,avg50\n")
                for row in pixel_data:
                    line = ",".join(str(x) for x in row)
                    f.write(line + "\n")
        print(f"💾 儲存 pixel 資料至 {filepath}")

def image_display_loop():
    while True:
        with frame_lock:
            if latest_frame is not None:
                cv2.imshow("Received Image", latest_frame)
        if cv2.waitKey(1) & 0xFF == ord('q'):
            break
        time.sleep(0.01)
    cv2.destroyAllWindows()

# 啟動接收與儲存執行緒
threading.Thread(target=pixel_receiver, daemon=True).start()
threading.Thread(target=pixel_saver, daemon=True).start()
threading.Thread(target=image_receiver, daemon=True).start()

# 啟動影像顯示迴圈
image_display_loop()
