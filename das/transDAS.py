import numpy as np

# 設定參數
input_file = 'demod_data_2025-07-30_10-43-02.bin'     # 你的 .bin 檔案路徑
output_file = 'demod_data_2025-07-30_10-43-02_output.txt'       # 輸出 .txt 檔案
num_channels = 1280              # 每個 frame 的通道數

# 讀取 .bin 檔案（float32 資料）
data = np.fromfile(input_file, dtype=np.float32)

# 重新形狀為 (N, 1280)
data = data.reshape(-1, num_channels)

# 儲存為 .txt，一行一組 frame
np.savetxt(output_file, data, fmt='%.6f', delimiter='\t')

print(f"成功轉換為 {output_file}，共 {data.shape[0]} 筆 frame。")
