import time
from datetime import datetime
from pywinauto import Desktop, Application
import win32gui
import win32con

# ================= 基本設定 =================
BLASTWARE_TITLE_RE = "Blastware"
BLASTWARE_PATH = r"C:\Blastware 10\Blastware.exe"
COPY_INDEX = 0
YES_TEXT = "Yes"
MAX_WAIT_SEC = 15 * 60
POLL_INTERVAL = 1.0
# ============================================


def log(msg):
    ms = int(datetime.now().microsecond / 10000)  # 微秒除以 10000 → 0~99
    print(f"[{datetime.now().strftime('%H:%M:%S')}.{ms:02d}] {msg}")


def find_button_by_text(parent, target_text):
    for btn in parent.descendants(control_type="Button"):
        if target_text.lower() in btn.window_text().lower():
            return btn
    return None


def wait_for_copy_finished():
    log("等待複製完成訊息（最多 15 分鐘）...")
    start = time.time()

    while time.time() - start < MAX_WAIT_SEC:
        def enum_windows(hwnd, result):
            if win32gui.IsWindowVisible(hwnd) \
               and win32gui.GetClassName(hwnd) == "#32770" \
               and win32gui.GetWindowText(hwnd) == "Blastware":
                result.append(hwnd)

        dialogs = []
        win32gui.EnumWindows(enum_windows, dialogs)

        for hwnd in dialogs:
            texts = []

            def enum_child(h, _):
                if win32gui.GetClassName(h) == "Static":
                    texts.append(win32gui.GetWindowText(h))

            win32gui.EnumChildWindows(hwnd, enum_child, None)
            message = " ".join(texts)
            log(f"偵測到對話框內容: {message}")

            if "Event" in message and "Copied" in message:
                log("確認為複製完成視窗，按下「確定」")

                def click_ok(h, _):
                    if win32gui.GetClassName(h) == "Button":
                        win32gui.SendMessage(h, win32con.BM_CLICK, 0, 0)

                win32gui.EnumChildWindows(hwnd, click_ok, None)
                log("複製流程正式完成")
                return True

        time.sleep(POLL_INTERVAL)

    raise TimeoutError("等待複製完成逾時（15 分鐘）")


def main():
    log("連接 Blastware (win32)...")
    desktop = Desktop(backend="win32")

    try:
        blastware = desktop.window(title_re=BLASTWARE_TITLE_RE)
        blastware.wait("visible", timeout=5)
        log("已找到啟動中的 Blastware")
    except:
        log("未找到啟動中的 Blastware，啟動中...")
        Application(backend="win32").start(BLASTWARE_PATH)
        time.sleep(15)
        blastware = desktop.window(title_re=BLASTWARE_TITLE_RE)
        blastware.wait("visible", timeout=30)
        log("Blastware 已啟動")

    blastware.set_focus()
    time.sleep(1)

    toolbars = blastware.descendants(class_name="ToolbarWindow32")
    if not toolbars:
        raise RuntimeError("找不到 ToolbarWindow32")

    log("點擊 Copy/Print")
    toolbars[0].button(COPY_INDEX).click_input()

    time.sleep(5)

    log("等待並點擊 Yes 按鈕...")
    for dlg in Desktop(backend="uia").windows():
        btn = find_button_by_text(dlg, YES_TEXT)
        if btn:
            log(f"找到 Yes 於視窗 '{dlg.window_text()}'，點擊")
            btn.invoke()
            break
    else:
        log("找不到 Yes 對話框，流程中止")
        return

    log("Copy/Print 已確認")
    wait_for_copy_finished()
    log("程式完成，即將關閉程式 ...")
    time.sleep(5)


if __name__ == "__main__":
    main()
