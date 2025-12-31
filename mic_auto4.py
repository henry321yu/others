import time
from pywinauto import Desktop, Application
import win32gui
import win32con

BLASTWARE_TITLE_RE = "Blastware"
BLASTWARE_PATH = r"C:\Blastware 10\Blastware.exe"
COPY_INDEX = 0
YES_TEXT = "Yes"
CONFIRM_TEXT = "確定"  # 中文確定按鈕文字
MAX_WAIT_SEC = 15 * 60   # 15 分鐘
POLL_INTERVAL = 1.0     # 每秒掃描一次

def find_button_by_text(parent, target_text):
    """在 parent 元件中找文字包含 target_text 的 Button"""
    buttons = parent.descendants(control_type="Button")
    for btn in buttons:
        text = btn.window_text().strip()
        if target_text.lower() in text.lower():
            return btn
    return None


def wait_for_copy_finished():
    print("等待複製完成訊息（最多 15 分鐘）...")

    start_time = time.time()

    while time.time() - start_time < MAX_WAIT_SEC:
        found = []

        def enum_windows(hwnd, results):
            if not win32gui.IsWindowVisible(hwnd):
                return

            title = win32gui.GetWindowText(hwnd)
            cls = win32gui.GetClassName(hwnd)

            # 只抓 Blastware 的 MessageBox
            if cls == "#32770" and title.strip() == "Blastware":
                results.append(hwnd)

        win32gui.EnumWindows(enum_windows, found)

        for hwnd in found:
            text = []

            def enum_child(hwnd_child, _):
                cls = win32gui.GetClassName(hwnd_child)
                if cls == "Static":
                    text.append(win32gui.GetWindowText(hwnd_child))

            win32gui.EnumChildWindows(hwnd, enum_child, None)

            full_text = " ".join(text)
            print(f"偵測到 Blastware 對話框文字: {full_text}")

            if "Event" in full_text and "Copied" in full_text:
                print("✅ 確認為複製完成視窗，嘗試按下「確定」")

                def click_ok(hwnd_child, _):
                    cls = win32gui.GetClassName(hwnd_child)
                    if cls == "Button":
                        win32gui.SendMessage(hwnd_child, win32con.BM_CLICK, 0, 0)

                win32gui.EnumChildWindows(hwnd, click_ok, None)

                print("✅ 已按下確定，複製流程正式完成")
                return True

        time.sleep(POLL_INTERVAL)

    raise TimeoutError("等待複製完成逾時（15 分鐘）")

def main():
    print("連接 Blastware (win32)...")
    desktop_win32 = Desktop(backend="win32")

    # 取得或啟動 Blastware
    try:
        blastware = desktop_win32.window(title_re=BLASTWARE_TITLE_RE)
        blastware.wait("visible", timeout=5)
        print("已找到 Blastware")
    except:
        print("未找到 Blastware，啟動中...")
        Application(backend="win32").start(BLASTWARE_PATH)
        time.sleep(6)
        blastware = desktop_win32.window(title_re=BLASTWARE_TITLE_RE)
        blastware.wait("visible", timeout=30)
        print("Blastware 已啟動")

    blastware.set_focus()
    # blastware.maximize()
    time.sleep(1)

    # 點擊 Copy / Print
    toolbars = blastware.descendants(class_name="ToolbarWindow32")
    if not toolbars:
        raise RuntimeError("找不到 ToolbarWindow32")
    toolbar = toolbars[0]
    print(f"點擊 Toolbar index={COPY_INDEX}（Copy / Print）")
    toolbar.button(COPY_INDEX).click_input()
    print("Copy/Print 已觸發")

    # 等待 5 秒讓 Copy/Print 對話框出現
    time.sleep(5)

    # 點擊 Yes
    print("等待並點擊 Yes 按鈕...")
    desktop_uia = Desktop(backend="uia")
    dialogs = desktop_uia.windows()
    yes_clicked = False
    for dlg in dialogs:
        btn_yes = find_button_by_text(dlg, YES_TEXT)
        if btn_yes:
            print(f"找到 Yes 按鈕在視窗 '{dlg.window_text()}'，點擊")
            btn_yes.invoke()
            yes_clicked = True
            break

    if not yes_clicked:
        print("找不到 Yes 按鈕，請確認是否有彈窗")
        time.sleep(5)
        return
    else:
        print("Copy/Print 對話框已確認")

    wait_for_copy_finished()
    print("程式完成，即將退出 ...")
    time.sleep(5)

if __name__ == "__main__":
    main()
