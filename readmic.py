import numpy as np
import matplotlib.pyplot as plt
import re

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
if "Pre-trigger_Length" in meta:
    preT = float(meta["Pre-trigger_Length"].replace("sec", "").strip())


# ---------- 時間軸 ----------
N = len(MicL1)
t = np.arange(N) / Fs + preT


# ---------- 最大值 ----------
idx = np.argmax(np.abs(MicL1))
maxMic = MicL1[idx]

# ---------- 印出 Metadata ----------
print("\n===== Event Metadata =====")
print(f"Event Type   : {meta.get('Event_Type','')}")
print(f"Event Date   : {meta.get('Event_Date','')}")
print(f"Event Time   : {meta.get('Event_Time','')}")
print(f"Trigger      : {meta.get('Trigger','')}")
print(f"Sample Rate  : {Fs:.0f} sps")
print(f"Record Time  : {meta.get('Record_Time','')}")
print(f"Pre-trigger  : {preT:.3f} sec")
print(f"Mic Peak     : {MicL1[idx]:.3f} Pa @ {t[idx]:.4f} sec")
print("===========================\n")

# ================= 繪圖 =================
plt.figure(figsize=(10, 4))
plt.plot(t, MicL1, linewidth=1.2)
plt.plot(t[idx], MicL1[idx], 'ro')
plt.grid(True)

plt.xlabel("Time (sec)")
plt.ylabel("Mic Pressure (Pa)")
plt.title("MiniMate Plus – MicL1 Full Waveform")

plt.text(
    t[idx], MicL1[idx],
    f"  Peak = {MicL1[idx]:.3f} Pa @ {t[idx]:.4f} s",
    verticalalignment="bottom"
)

plt.tight_layout()
plt.show()
# ======================================