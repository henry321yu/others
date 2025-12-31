import time
from pywinauto import Desktop, Application

BLASTWARE_TITLE_RE = "Blastware"
BLASTWARE_PATH = r"C:\Blastware 10\Blastware.exe"
COPY_INDEX = 0
YES_TEXT = "Yes"

def find_button_by_text(parent, target_text):
    """在 parent 元件中找文字包含 target_text 的 Button"""
    buttons = parent.descendants(control_type="Button")
    for btn in buttons:
        text = btn.window_text().strip()
        if target_text.lower() in text.lower():
            return btn
    return None

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
    blastware.maximize()
    time.sleep(1)

    # 找 ToolbarWindow32
    toolbars = blastware.descendants(class_name="ToolbarWindow32")
    if not toolbars:
        raise RuntimeError("找不到 ToolbarWindow32")

    toolbar = toolbars[0]
    print(f"點擊 Toolbar index={COPY_INDEX}（Copy / Print）")
    toolbar.button(COPY_INDEX).click_input()
    print("✅ Copy / Print 已觸發")

    # 等待 5 秒讓對話框出現
    time.sleep(5)

    # 使用 UIA backend 自動點擊 Yes
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
        print("⚠️ 找不到 Yes 按鈕，請確認是否有彈窗")

    print("✅ 流程完成")

if __name__ == "__main__":
    main()
