import socket
import struct
import cv2
import numpy as np
import time
import os
from datetime import datetime
import subprocess

LISTEN_IP = "0.0.0.0"
LISTEN_PORT = 5200

SAVE_DIR = "videos"
os.makedirs(SAVE_DIR, exist_ok=True)

SEGMENT_DURATION = 600  # 10分鐘

ffmpeg_process = None
segment_start_time = 0
recording = False  # 啟動就錄影

FFMPEG_PATH = r"C:\ffmpeg\bin\ffmpeg.exe"   # ← 你的 ffmpeg 路徑，已設定完畢

def start_ffmpeg_writer(width, height):
    global segment_start_time

    timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
    filename = os.path.join(SAVE_DIR, f"record_{timestamp}.mkv")

    print(f"開始錄影 → {filename}")

    segment_start_time = time.time()

    return subprocess.Popen(
        [
            FFMPEG_PATH,         # ← 使用完整路徑，不會再 WinError 2
            "-y",
            "-f", "rawvideo",
            "-pix_fmt", "bgr24",
            "-s", f"{width}x{height}",
            "-r", "8",                  # ← 這裡不要填 1000，會造成快轉！
            "-i", "-",
            "-c:v", "libx264",
            "-preset", "veryfast",
            "-crf", "23",
            "-vsync", "cfr",             # ← 強制「固定每秒幀數」，不會快轉
            filename
        ],
        stdin=subprocess.PIPE
    )


def handle_client(conn, addr):
    global ffmpeg_process, recording, segment_start_time

    print(f"Client connected: {addr}")

    cv2.namedWindow("Received")

    try:
        frame_width = None
        frame_height = None

        while True:
            header = recv_all(conn, 4)
            if not header:
                print("客戶端已關閉連線")
                break

            (length,) = struct.unpack(">I", header)
            jpg_data = recv_all(conn, length)
            if not jpg_data:
                print("JPEG 讀取中斷")
                break

            frame = cv2.imdecode(np.frombuffer(jpg_data, np.uint8), cv2.IMREAD_COLOR)
            if frame is None:
                print("JPEG 解碼失敗")
                continue

            # 顯示
            title = "Realtime screen (control:q=quit, r=toggle record)"
            if recording:
                elapsed = int(time.time() - segment_start_time)
                mm = elapsed // 60
                ss = elapsed % 60
                rec_time = f"{mm:02d}:{ss:02d}"
                title = f"Realtime screen recording ... {rec_time}  (control:q=quit, r=toggle record)"

            cv2.setWindowTitle("Received", title)
            cv2.imshow("Received", frame)

            key = cv2.waitKey(1) & 0xFF

            # 記錄畫面尺寸
            if frame_width is None:
                frame_height, frame_width = frame.shape[:2]
                ffmpeg_process = start_ffmpeg_writer(frame_width, frame_height)

            # r 鍵切換錄影
            if key == ord('r'):
                recording = not recording
                if recording:
                    print("恢復錄影")
                    ffmpeg_process = start_ffmpeg_writer(frame_width, frame_height)
                else:
                    print("暫停錄影")
                    if ffmpeg_process:
                        ffmpeg_process.stdin.close()
                        ffmpeg_process.wait()
                        ffmpeg_process = None

            # q 離開
            if key == ord('q'):
                break

            # 寫入 FFmpeg（真實時間）
            if recording and ffmpeg_process:
                ffmpeg_process.stdin.write(frame.tobytes())

                # 10 分鐘切檔
                if time.time() - segment_start_time >= SEGMENT_DURATION:
                    print("10分鐘到，自動切換新檔案")
                    ffmpeg_process.stdin.close()
                    ffmpeg_process.wait()
                    ffmpeg_process = start_ffmpeg_writer(frame_width, frame_height)

    finally:
        if ffmpeg_process:
            ffmpeg_process.stdin.close()
            ffmpeg_process.wait()
        conn.close()
        print(f"Client {addr} disconnected")


def recv_all(sock, length):
    data = b""
    while len(data) < length:
        packet = sock.recv(length - len(data))
        if not packet:
            return None
        data += packet
    return data


def main():
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.bind((LISTEN_IP, LISTEN_PORT))
    sock.listen(1)

    print(f"Listening on {LISTEN_IP}:{LISTEN_PORT} ... ")

    while True:
        conn, addr = sock.accept()
        handle_client(conn, addr)


if __name__ == "__main__":
    main()
