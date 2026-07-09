#!/usr/bin/env python3
# win_receiver.py
# TCP server 接收 JPEG 幀，顯示 + 自動錄影 + 10 分鐘分段檔案

import socket
import struct
import cv2
import numpy as np
import time
import os
from datetime import datetime

LISTEN_IP = "0.0.0.0"
LISTEN_PORT = 5200

# ★ 設定錄影資料夾
SAVE_DIR = "videos"
os.makedirs(SAVE_DIR, exist_ok=True)

# ★ 錄影 FPS（自行調整）
RECORD_FPS = 1

# ★ 每段影片時長（秒）：10 分鐘 = 600 秒
SEGMENT_DURATION = 600


def recv_all(sock, length):
    data = b""
    while len(data) < length:
        packet = sock.recv(length - len(data))
        if not packet:
            return None
        data += packet
    return data


def create_new_writer(frame):
    """建立新的錄影檔案並回傳 VideoWriter"""
    timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
    filename = os.path.join(SAVE_DIR, f"record_{timestamp}.avi")

    height, width, _ = frame.shape
    fourcc = cv2.VideoWriter_fourcc(*"MJPG")
    writer = cv2.VideoWriter(filename, fourcc, RECORD_FPS, (width, height))

    print(f"新影片開始錄製 → {filename}")
    return writer, time.time()  # 回傳 writer 和開檔時間


def handle_client(conn, addr):
    print(f"Client connected: {addr}")

    video_writer = None
    recording = True  # ★ 啟動後立即錄影
    last_record_time = 0
    segment_start_time = 0  # 用來切換 10 分鐘影片

    try:
        while True:
            header = recv_all(conn, 4)
            if not header:
                print("客戶端已關閉連線")
                break

            (length,) = struct.unpack(">I", header)
            jpg_data = recv_all(conn, length)
            if not jpg_data:
                print("在讀取 JPEG 時連線中斷")
                break

            np_arr = np.frombuffer(jpg_data, dtype=np.uint8)
            frame = cv2.imdecode(np_arr, cv2.IMREAD_COLOR)
            if frame is None:
                print("JPEG 解碼失敗")
                continue

            # 若是第一次收到 frame，就開啟第一個錄影檔
            if video_writer is None and recording:
                video_writer, segment_start_time = create_new_writer(frame)

            # ---- 顯示影像 ----
            cv2.imshow("Received (q=exit, r=toggle record)", frame)
            key = cv2.waitKey(1) & 0xFF

            # ---- 錄影開關 ----
            if key == ord('r'):
                recording = not recording

                if recording:
                    print("恢復錄影")
                    video_writer, segment_start_time = create_new_writer(frame)
                else:
                    print("錄影已暫停")
                    if video_writer:
                        video_writer.release()
                        video_writer = None

            # ---- 寫入影片 ----
            if recording and video_writer:
                now = time.time()

                # FPS 控制
                if now - last_record_time >= 1.0 / RECORD_FPS:
                    video_writer.write(frame)
                    last_record_time = now

                # 判斷是否需切換新影片（10 分鐘）
                if now - segment_start_time >= SEGMENT_DURATION:
                    print("達到 10 分鐘，自動切換新影片")

                    video_writer.release()
                    video_writer, segment_start_time = create_new_writer(frame)

            # ---- 離開 ----
            if key == ord('q'):
                print("使用者要求結束")
                break

    finally:
        if video_writer:
            video_writer.release()
        conn.close()
        print(f"Client {addr} disconnected")


def main():
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.bind((LISTEN_IP, LISTEN_PORT))
    sock.listen(1)
    print(f"Listening on {LISTEN_IP}:{LISTEN_PORT} ... (按 Ctrl+C 停止)")

    try:
        while True:
            conn, addr = sock.accept()
            handle_client(conn, addr)

    except KeyboardInterrupt:
        print("停止伺服器")
    finally:
        sock.close()
        cv2.destroyAllWindows()


if __name__ == "__main__":
    main()
