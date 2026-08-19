import ctypes
import subprocess
import time

SERVICE_NAME = "wuauserv"


def is_admin():
    """確認是否具有系統管理員權限"""
    try:
        return ctypes.windll.shell32.IsUserAnAdmin()
    except:
        return False


def run_as_admin():
    """重新以系統管理員執行自己"""
    ctypes.windll.shell32.ShellExecuteW(
        None,
        "runas",
        sys.executable,
        " ".join(f'"{arg}"' for arg in sys.argv),
        None,
        1
    )


def disable_windows_update():
    # 停止 Windows Update
    subprocess.run(
        ["sc", "stop", SERVICE_NAME],
        capture_output=True,
        text=True
    )

    # 將啟動類型設定為 Disabled
    subprocess.run(
        ["sc", "config", SERVICE_NAME, "start=", "disabled"],
        capture_output=True,
        text=True
    )


if __name__ == "__main__":
    import sys

    if not is_admin():
        print("需要系統管理員權限，正在要求權限...")
        run_as_admin()
        sys.exit()

    print("正在停用 Windows Update...")
    disable_windows_update()

    print("Windows Update 已設定為停用。")

    # 開啟服務管理員
    subprocess.Popen(["mmc.exe", "services.msc"])

    time.sleep(1)