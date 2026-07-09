import socket
import cv2
import numpy as np

# 設定接收的本機 IP 與 port（與發送端一致）
UDP_IP = "0.0.0.0"       # 接收所有網卡的資料
UDP_PORT = 2885          # 必須與發送端使用的影像 port 相同

# 建立 socket
sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sock.bind((UDP_IP, UDP_PORT))

print(f"🎥 等待接收影像資料 on UDP port {UDP_PORT}...")

while True:
    try:
        data, addr = sock.recvfrom(65535)  # 接收 UDP 封包 (最大 65535 bytes)
        np_arr = np.frombuffer(data, dtype=np.uint8)
        frame = cv2.imdecode(np_arr, cv2.IMREAD_COLOR)

        if frame is not None:
            cv2.imshow("UDP Received Camera Feed", frame)
            if cv2.waitKey(1) & 0xFF == ord('q'):
                break
    except Exception as e:
        print(f"⚠️ 接收或解碼錯誤: {e}")

sock.close()
cv2.destroyAllWindows()