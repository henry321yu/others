import time
import pyautogui
from pywinauto.application import Application
from pywinauto import Desktop

# -----------------------------
# 1. 嘗試連接已開啟的 Blastware
# -----------------------------
try:
    # 嘗試抓現有 Blastware 視窗
    dlg = Desktop(backend="win32").window(title_re="Blastware")
    dlg.wait("visible", timeout=5)
    print("已找到開啟中的 Blastware")
except:
    # 如果找不到，啟動 Blastware
    blastware_path = r"C:\Blastware 10\Blastware.exe"
    app = Application(backend="win32").start(blastware_path)
    time.sleep(5)  # 等待程式啟動
    dlg = Desktop(backend="win32").window(title_re="Blastware")
    dlg.wait("visible", timeout=30)
    print("Blastware 已啟動")

# -----------------------------
# 2. 將視窗拉到前景並最大化
# -----------------------------
dlg.set_focus()
dlg.maximize()
time.sleep(1)
print("Blastware 視窗已最大化")

dlg.print_control_identifiers()
# -----------------------------
# 3. 點擊 Copy/Print 按鈕
# -----------------------------
copy_print_coords = (38, 132)  # 最大化後座標自行調整
pyautogui.click(*copy_print_coords)
print("已點擊 Copy/Print")
time.sleep(10)

# -----------------------------
# 4. 點擊 Yes 按鈕
# -----------------------------
yes_button_coords = (913, 612)  # 最大化後座標自行調整
pyautogui.click(*yes_button_coords)
print("已點擊 Yes，開始下載 MiniMate 檔案")

time.sleep(1)
print("MiniMate 檔案已下載到本機")
