import socket

# 這裡填入電腦 B 的 Radmin IP
TARGET_IP = '26.55.45.86'
PORTS_TO_TEST = [1688, 25734]

while True:
    for port in PORTS_TO_TEST:
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s.settimeout(3) # 設定 3 秒超時
        try:
            s.connect((TARGET_IP, port))
            print(f"[成功] 已經連上 {TARGET_IP}:{port}，轉發通道正常！")
        except Exception as e:
            print(f"[失敗] 無法連線到 {TARGET_IP}:{port}，錯誤: {e}")
        finally:
            s.close()