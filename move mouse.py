import pyautogui
import time

def move_mouse():
    while True:
        # 取得目前滑鼠位置
        x, y = pyautogui.position()
        # 稍微移動一下滑鼠（右移 1 像素，再左移回來）
        pyautogui.moveTo(x + 1, y)
        pyautogui.moveTo(x, y)
        print("滑鼠移動了一下")
        # 等待 15 分鐘（900 秒）
        time.sleep(300)

if __name__ == "__main__":
    print("開始防螢幕保護程式...")
    move_mouse()
