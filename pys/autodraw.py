import pyautogui
import time
import subprocess

# 開啟小畫家
subprocess.Popen('mspaint')

# 等待小畫家啟動
time.sleep(5)

i=0
sw, sh = pyautogui.size() #得到螢幕長寬
drawspeed=0 #繪畫速度
d=500 #寬度
md=1 #縮減距離

pyautogui.moveTo(sw/2+d/2, sh/2-d/2, duration=1) #make it draw at the center

while d>0:
    pyautogui.drag(0,d, duration=drawspeed)
    pyautogui.drag(-d,0, duration=drawspeed)
    d-=md
    pyautogui.drag(0,-d, duration=drawspeed)
    pyautogui.drag(d,0, duration=drawspeed)
    d-=md

exit()