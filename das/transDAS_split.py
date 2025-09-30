import numpy as np
import os

# ====== 設定 ======
input_file = 'demod_data_2025-07-30_10-43-02.bin'  # 原始 bin 檔名
num_channels = 1280
frames_per_file = 2000
output_dir = 'DAS data'  # 輸出資料夾

# 建立資料夾（如果尚未存在）
os.makedirs(output_dir, exist_ok=True)

# 取得檔名前綴（不含副檔名）
base_name = os.path.splitext(os.path.basename(input_file))[0]

# 讀取全部資料
data = np.fromfile(input_file, dtype=np.float32)
total_frames = data.size // num_channels

# 重新整理形狀
data = data.reshape(-1, num_channels)

# 分段輸出
for i in range(0, total_frames, frames_per_file):
    chunk = data[i:i + frames_per_file]
    index = i // frames_per_file
    output_filename = os.path.join(output_dir, f'{base_name}_{index}.txt')
    np.savetxt(output_filename, chunk, fmt='%.6f', delimiter='\t')
    print(f"輸出 {output_filename}，frame {i} ~ {i + len(chunk) - 1}")
