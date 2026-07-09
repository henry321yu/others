#!/usr/bin/env python3
# win_receiver.py
# Windows 10: TCP server 接收長度前綴的 JPEG 幀，解碼並顯示
# 要求: python3, opencv-python

import socket
import struct
import cv2
import numpy as np

LISTEN_IP = "0.0.0.0"   # 接收所有介面（ZeroTier 介面也會監聽）
LISTEN_PORT = 5200

def recv_all(sock, length):
    """從 socket 讀取固定長度的 bytes"""
    data = b""
    while len(data) < length:
        packet = sock.recv(length - len(data))
        if not packet:
            return None
        data += packet
    return data

def handle_client(conn, addr):
    print(f"Client connected: {addr}")
    try:
        while True:
            # 先讀 4 bytes 長度
            header = recv_all(conn, 4)
            if not header:
                print("客戶端已關閉連線")
                break
            (length,) = struct.unpack(">I", header)
            # 讀取 length bytes 的 JPEG
            jpg_data = recv_all(conn, length)
            if not jpg_data:
                print("在讀取 JPEG 時連線中斷")
                break

            # 轉成 numpy array 並解碼
            np_arr = np.frombuffer(jpg_data, dtype=np.uint8)
            frame = cv2.imdecode(np_arr, cv2.IMREAD_COLOR)  # 彩色
            if frame is None:
                print("JPEG 解碼失敗")
                continue

            # 顯示（會自動維持彩色）
            cv2.imshow("Received (press q to quit)", frame)
            if cv2.waitKey(1) & 0xFF == ord('q'):
                print("使用者要求結束")
                return
    finally:
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
