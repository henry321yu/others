# file_receiver.py（電腦B執行）
import socket
import os

FOLDER = os.path.expanduser("~/Desktop/sync_data")
PORT = 5001

os.makedirs(FOLDER, exist_ok=True)

with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
    s.bind(('', PORT))
    s.listen(5)
    print(f"[Receiver] Listening on port {PORT}...")

    while True:
        conn, addr = s.accept()
        with conn:
            # 取得檔名長度與檔名
            name_len = int.from_bytes(conn.recv(4), 'big')
            filename = conn.recv(name_len).decode()

            # 取得檔案大小
            filesize = int.from_bytes(conn.recv(8), 'big')
            file_path = os.path.join(FOLDER, filename)

            # 接收並寫入檔案
            with open(file_path, 'wb') as f:
                received = 0
                while received < filesize:
                    data = conn.recv(min(4096, filesize - received))
                    if not data:
                        break
                    f.write(data)
                    received += len(data)

            print(f"[Receiver] File received: {filename}")
