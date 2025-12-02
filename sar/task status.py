import subprocess
import time
from datetime import datetime
from collections import OrderedDict

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
# 欄位對應中文欄位名稱（用於 print）
CHINESE_LABELS = {
    "TaskName": "工作名稱",
    "Status": "狀態",
    "LastRunTime": "上次執行時間",
    "TaskToRun": "執行工作",
    "StartIn": "開始位置",
    "Comment": "註解",
}

def get_task_status():
    cmd = f'schtasks /query /fo LIST /v /tn {TASK_NAME}'
    output = subprocess.check_output(cmd, shell=True, encoding="cp950", errors="ignore")

    # 使用 OrderedDict 保持欄位順序
    field_order = ["TaskName", "Status", "LastRunTime", "TaskToRun", "StartIn", "Comment"]
    result = OrderedDict((field, "") for field in field_order)

    for line in output.splitlines():
        if ":" not in line:
            continue

        key_raw, value = line.split(":", 1)
        key_raw = key_raw.strip().replace("\u3000", "")
        value = value.strip()

        for k in KEY_MAP:
            if k in key_raw:
                if not result[KEY_MAP[k]]:
                    result[KEY_MAP[k]] = value

    return result

def live_monitor(interval=1):
    while True:
        status = get_task_status()
        now = datetime.now().strftime("%Y/%m/%d %H:%M:%S")

        print(f"=== ArcSAR 任務即時狀態 {now} ===")
        for k, v in status.items():
            print(f"{CHINESE_LABELS[k]}: {v}")

        time.sleep(interval)

if __name__ == "__main__":
    live_monitor()
