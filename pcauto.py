import time
import pyautogui

print("等5秒")
time.sleep(5)
while True:
    print("按下 →")
    pyautogui.press('right')
    # pyautogui.press('left')
    time.sleep(35)