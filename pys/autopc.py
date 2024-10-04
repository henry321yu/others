import pyautogui
import time
import subprocess

# 開啟小畫家
subprocess.Popen('mspaint')

# 等待小畫家啟動
time.sleep(3)

i=0
sw, sh = pyautogui.size()
drawspeed=0.001
d=300
md=2

pyautogui.moveTo(sw/2, sh/2, duration=1)

while d>0:
    pyautogui.drag(d,0, duration=drawspeed)
    d-=md
    pyautogui.drag(0,d, duration=drawspeed)
    pyautogui.drag(-d,0, duration=drawspeed)
    d-=md
    pyautogui.drag(0,-d, duration=drawspeed)

exit()