import time
from pywinauto import Desktop, Application

# -----------------------------
# 設定區
# -----------------------------
BLASTWARE_TITLE_RE = "Blastware"
EVENT_MANAGER_TITLE_RE = "Event Manager"
COPY_PRINT_TEXT = "Copy"
YES_TEXT = "Yes"
BLASTWARE_PATH = r"C:\Blastware 10\Blastware.exe"

# -----------------------------
# 工具函式
# -----------------------------
def find_event_manager(blastware_window):
    """
    找到 Blastware 的 Event Manager 子視窗
    並印出搜尋過程中每個子視窗的 window_text 與 control_type
    """
    children = blastware_window.descendants(control_type="Window")
    print(f"總共找到 {len(children)} 個子視窗候選:")
    
    for i, child in enumerate(children):
        print(f"[{i}] handle={child.handle}, text='{child.window_text()}', type={child.friendly_class_name()}")
        if EVENT_MANAGER_TITLE_RE.lower() in child.window_text().lower():
            print(f"✅ 找到符合條件的 Event Manager: handle={child.handle}, text='{child.window_text()}'")
            return child
    
    print("❌ 沒有找到符合條件的 Event Manager")
    return None
    
def find_button_by_text(parent, target_text):
    """
    在 parent 元件中找文字包含 target_text 的 Button
    並印出搜尋過程
    """
    buttons = parent.descendants(control_type="Button")
    print(f"總共找到 {len(buttons)} 個 Button 候選:")
    
    for i, btn in enumerate(buttons):
        text = btn.window_text().strip()
        print(f"[{i}] handle={btn.handle}, text='{text}'")
        if target_text.lower() in text.lower():
            print(f"✅ 找到符合條件的按鈕: handle={btn.handle}, text='{text}'")
            return btn

    print(f"❌ 沒有找到文字包含 '{target_text}' 的按鈕")
    return None


# -----------------------------
# 主流程
# -----------------------------
def main():
    print("嘗試連接 Blastware ...")
    desktop = Desktop(backend="uia")

    # -----------------------------
    # 嘗試抓現有 Blastware 視窗
    # -----------------------------
    try:
        blastware = desktop.window(title_re=BLASTWARE_TITLE_RE)
        blastware.wait("visible", timeout=5)
        print("已找到開啟中的 Blastware")
    except:
        print("找不到 Blastware，啟動程式中...")
        app = Application(backend="win32").start(BLASTWARE_PATH)
        time.sleep(5)
        blastware = desktop.window(title_re=BLASTWARE_TITLE_RE)
        blastware.wait("visible", timeout=30)
        print("Blastware 已啟動")

    # -----------------------------
    # 最大化 Blastware
    # -----------------------------
    blastware.set_focus()
    blastware.maximize()
    print("Blastware 已最大化")

    # -----------------------------
    # 找 Event Manager 子視窗
    # -----------------------------
    print("尋找 Event Manager 子視窗 ...")
    event_mgr = find_event_manager(blastware)
    if not event_mgr:
        print("❌ 找不到 Event Manager")
        return
    print(f"已找到 Event Manager, handle={event_mgr.handle}")

    # -----------------------------
    # 找 Copy/Print 按鈕
    # -----------------------------
    copy_btn = find_button_by_text(event_mgr, COPY_PRINT_TEXT)
    if not copy_btn:
        print("❌ 找不到 Copy/Print 按鈕")
        return
    print(f"找到 Copy/Print 按鈕: {copy_btn.window_text()}，開始點擊")
    copy_btn.invoke()
    time.sleep(1.5)  # 等待對話框彈出

    # -----------------------------
    # 點擊 Yes 按鈕
    # -----------------------------
    print("等待並點擊 Yes 按鈕...")
    dialogs = Desktop(backend="uia").windows()
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

    print("流程完成")

# -----------------------------
# 進入點
# -----------------------------
if __name__ == "__main__":
    main()
