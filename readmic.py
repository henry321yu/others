import numpy as np
import matplotlib.pyplot as plt
import re
from pathlib import Path

# ================= 使用者設定 =================
file = r"C:\Blastware 10\Event\ascii\ASCII.TXT"
Fs_default = 1024
# =============================================

file = Path(file)

if not file.exists():
    raise FileNotFoundError("無法開啟檔案")

# ================= 初始化 =================
meta = {}
data = []
read_data = False
channel_names = []
nCh = 0

# ================= 逐行讀檔 =================
with open(file, "r", encoding="utf-8", errors="ignore") as f:
    for raw_line in f:
        line = raw_line.strip()
        if not line:
            continue

        # ---------- Metadata ----------
        if line.startswith('"') and not read_data:
            line = line.replace('"', '')
            m = re.match(r'^(.*?)\s*:\s*(.*)$', line)
            if m:
                key = re.sub(r'\W|^(?=\d)', '_', m.group(1).strip())
                val = m.group(2).strip()
                meta[key] = val

        # ---------- 欄位名稱（資料起始） ----------
        elif not read_data:
            names = line.split()
            if (any("mic" in n.lower() for n in names) or
                any(k.lower() in n.lower() for n in names
                    for k in ["tran", "vert", "long"])):

                channel_names = names
                nCh = len(channel_names)
                read_data = True

        # ---------- 波形資料 ----------
        elif read_data:
            nums = np.fromstring(line, sep=' ')
            if len(nums) == nCh:
                data.append(nums)

data = np.array(data)

# ================= Sample Rate =================
if "Sample_Rate" in meta:
    Fs = float(meta["Sample_Rate"].replace("sps", ""))
else:
    Fs = Fs_default

# ================= Pre-trigger =================
preT = 0.0
if "Pre_trigger_Length" in meta:
    preT = float(meta["Pre_trigger_Length"].replace("sec", ""))

# ================= 時間軸 =================
N = data.shape[0]
t = np.arange(N) / Fs + preT

# ================= 單位判斷 =================
units = []
for name in channel_names:
    if "mic" in name.lower():
        units.append("Pa")
    else:
        units.append("mm/s")

# ================= 繪圖 =================
for ch in range(nCh):
    sig = data[:, ch]
    idx = np.argmax(np.abs(sig))
    pk = sig[idx]

    plt.figure(figsize=(9, 4))
    plt.plot(t, sig, linewidth=1.2)
    plt.plot(t[idx], sig[idx], 'ro')

    plt.grid(True)
    plt.xlabel("Time (sec)")
    plt.ylabel(f"{channel_names[ch]} ({units[ch]})")

    title = (
        "MiniMate Plus – Full Waveform\n"
        f"{channel_names[ch]}   "
        f"Date: {meta.get('EventDate','')}   "
        f"Time: {meta.get('EventTime','')}"
    )
    plt.title(title)

    plt.text(
        t[idx], sig[idx],
        f"  Peak = {pk:.3f} {units[ch]} @ {t[idx]:.3f} s",
        verticalalignment="bottom"
    )

    plt.tight_layout()
plt.show()
