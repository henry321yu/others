import numpy as np
import matplotlib.pyplot as plt
import re
from datetime import datetime, timedelta

# ================= 使用者設定 =================
file = r"C:\Blastware 10\Event\ascii\ASCII.TXT"
Fs_default = 1024.0   # 備援 sample rate
# =============================================

# ---------- 初始化 ----------
meta = {}
MicL1 = []
read_data = False

# ---------- 讀檔 ----------
with open(file, "r", encoding="utf-8", errors="ignore") as f:
    for raw in f:
        line = raw.strip()
        if not line:
            continue

        # ---------- Metadata ----------
        if line.startswith('"'):
            line = line.replace('"', '')
            m = re.match(r'^(.*?)\s*:\s*(.*)$', line)
            if m:
                key = m.group(1).strip().replace(' ', '_')
                val = m.group(2).strip()
                meta[key] = val

        # ---------- 欄位名稱 ----------
        elif line == "MicL1":
            read_data = True

        # ---------- 波形資料 ----------
        elif read_data:
            try:
                MicL1.append(float(line))
            except ValueError:
                pass

MicL1 = np.asarray(MicL1)

# ---------- Sample Rate ----------
if "Sample_Rate" in meta:
    Fs = float(meta["Sample_Rate"].replace("sps", "").strip())
else:
    Fs = Fs_default

# ---------- Pre-trigger ----------
preT = 0.0
if "Pre_trigger_Length" in meta:
    preT = float(meta["Pre_trigger_Length"].replace("sec", "").strip())

# ---------- 時間軸 ----------
N = len(MicL1)
t = np.arange(N) / Fs + preT

# ---------- 最大值 ----------
idx = np.argmax(np.abs(MicL1))
maxMic = MicL1[idx]

# ---------- 事件真實時間 ----------
eventDT = datetime.strptime(meta['Event_Time'], '%H:%M:%S')
peakDT = eventDT + timedelta(seconds=float(t[idx]))

# ---------- 印出 Metadata ----------
print("\n===== Event Metadata =====")
print(f"Event Type   : {meta.get('Event_Type','')}")
print(f"Event Date   : {meta.get('Event_Date','')}")
print(f"Event Time   : {meta.get('Event_Time','')}")
print(f"Trigger      : {meta.get('Trigger','')}")
print(f"Sample Rate  : {Fs:.0f} sps")
print(f"Record Time  : {meta.get('Record_Time','')}")
print(f"Pre-trigger  : {preT:.3f} sec")
print(f"Mic Peak     : {MicL1[idx]:.3f} Pa @ {peakDT.strftime('%H:%M:%S.%f')[:-3]}")
print("===========================\n")

# ================= 繪圖 =================
plt.figure(figsize=(10, 4))
plt.plot(t, MicL1, linewidth=1.2)
plt.plot(t[idx], MicL1[idx], 'ro')
plt.grid(True)

plt.xlabel("Time (sec)")
plt.ylabel("Mic Pressure (Pa)")

# ---------- Title 加入 metadata ----------
plt.title(f"MiniMate Plus – MicL1 Full Waveform\n"
          f"Date: {meta.get('Event_Date','')}  "
          f"Time: {meta.get('Event_Time','')}  "
          f"Record: {meta.get('Record_Time','')}  "
          f"Peak: {maxMic:.3f} Pa")

# ---------- 標註 Peak 真實時間 ----------
plt.text(t[idx], MicL1[idx],
         f"Peak = {maxMic:.3f} Pa @ {peakDT.strftime('%H:%M:%S.%f')[:-3]}",
         verticalalignment='bottom')

plt.tight_layout()
plt.show()
# ======================================
