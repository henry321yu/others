import pyautogui
import time
from datetime import datetime

def move_mouse():
    while True:
        # 取得目前滑鼠位置
        x, y = pyautogui.position()
        # 稍微移動滑鼠（右移 1 像素再移回來）
        pyautogui.moveTo(x + 1, y)
        pyautogui.moveTo(x, y)
        # 印出時間戳與狀態
        now = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
        print(f"[{now}] 滑鼠移動了一下")
        # 等待 15 分鐘（900 秒）
        time.sleep(300)

if __name__ == "__main__":
    print("開始防螢幕保護程式...")
    move_mouse()
