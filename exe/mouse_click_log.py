import os
from pynput import mouse
import time

# 設定初始的 log 檔案名稱和路徑
current_datetime = time.strftime("%Y%m%d_%H%M%S")
log_filename = f"mouse_click_log_{current_datetime}.txt"

# 取得當前執行的目錄，並在該目錄創建 log 檔案
log_file_path = os.path.join(os.getcwd(), log_filename)

# 記錄每個按鍵按下的次數
click_counts = {
    "left": 0,
    "right": 0,
    "middle": 0
}

# 當按鍵按下時執行
def on_click(x, y, button, pressed):
    if pressed:
        if button == mouse.Button.left:
            click_counts["left"] += 1
        elif button == mouse.Button.right:
            click_counts["right"] += 1
        elif button == mouse.Button.middle:
            click_counts["middle"] += 1
        
        # 寫入 log 檔
        try:
            with open(log_file_path, "a") as log:
                log.write(f"{time.strftime('%Y-%m-%d %H:%M:%S')} - Left: {click_counts['left']}, Right: {click_counts['right']}, Middle: {click_counts['middle']}\n")
        except PermissionError:
            print(f"PermissionError: Unable to write to {log_file_path}. Please check if the file is open or the directory has write permissions.")
            return

        # 顯示目前的按鍵次數
        print(f"{time.strftime('%Y-%m-%d %H:%M:%S')} - Left: {click_counts['left']}, Right: {click_counts['right']}, Middle: {click_counts['middle']}")

# 啟動監聽器
with mouse.Listener(on_click=on_click) as listener:
    listener.join()
