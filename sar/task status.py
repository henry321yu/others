import subprocess
import time
from datetime import datetime

TASK_NAME = "arcsar_auto"

# 關鍵字可能的多語系對應
KEY_MAP = {
    "工作名稱": "TaskName",
    "TaskName": "TaskName",

    "狀態": "Status",
    "Status": "Status",

    "上次執行時間": "LastRunTime",
    "Last Run Time": "LastRunTime",

    "執行工作": "TaskToRun",
    "Task To Run": "TaskToRun",

    "開始位置": "StartIn",
    "Start In": "StartIn",

    "註解": "Comment",
    "Comment": "Comment",
}

def get_task_status():
    cmd = f'schtasks /query /fo LIST /v /tn {TASK_NAME}'
    output = subprocess.check_output(cmd, shell=True, encoding="big5", errors="ignore")

    result = {v: "" for v in set(KEY_MAP.values())}

    for line in output.splitlines():
        if ":" not in line:
            continue

        key_raw, value = line.split(":", 1)
        key_raw = key_raw.strip()
        value = value.strip()

        # 逐項比對 "包含關鍵字"（避免空白、BOM 造成匹配失敗）
        for k in KEY_MAP:
            if k in key_raw:
                result[KEY_MAP[k]] = value

    return result


def live_monitor(interval=1):
    while True:
        status = get_task_status()
        now = datetime.now().strftime("%Y/%m/%d %H:%M:%S")

        print(f"=== ArcSAR 任務即時狀態 {now} ===")
        for k, v in status.items():
            print(f"{k}: {v}")

        time.sleep(interval)


if __name__ == "__main__":
    live_monitor()
