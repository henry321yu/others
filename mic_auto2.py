import time
from pywinauto import Desktop, Application

BLASTWARE_TITLE_RE = "Blastware"
BLASTWARE_PATH = r"C:\Blastware 10\Blastware.exe"

# ⚠️ Copy / Print 的 toolbar index（你之後只會改這行）
COPY_INDEX = 0

def main():
    print("連接 Blastware (win32)...")
    desktop = Desktop(backend="win32")

    # 取得或啟動 Blastware
    try:
        blastware = desktop.window(title_re=BLASTWARE_TITLE_RE)
        blastware.wait("visible", timeout=5)
        print("已找到 Blastware")
    except:
        print("未找到 Blastware，啟動中...")
        Application(backend="win32").start(BLASTWARE_PATH)
        time.sleep(6)
        blastware = desktop.window(title_re=BLASTWARE_TITLE_RE)
        blastware.wait("visible", timeout=30)
        print("Blastware 已啟動")

    blastware.set_focus()
    blastware.maximize()
    time.sleep(1)

    # 找 ToolbarWindow32
    print("搜尋 ToolbarWindow32 ...")
    toolbars = blastware.descendants(class_name="ToolbarWindow32")
    print(f"找到 {len(toolbars)} 個 ToolbarWindow32")

    if not toolbars:
        raise RuntimeError("找不到 ToolbarWindow32")

    toolbar = toolbars[0]
    print(f"使用 Toolbar handle={toolbar.handle}")

    # ★ 唯一正確的操作方式 ★
    print(f"點擊 Toolbar index={COPY_INDEX}（Copy / Print）")
    toolbar.button(COPY_INDEX).click_input()

    print("✅ Copy / Print 已觸發")
    
if __name__ == "__main__":
    main()
