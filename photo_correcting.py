import cv2
import numpy as np
import os
from PIL import Image

# 設定資料夾
input_folder = "A"
output_folder = "B"
os.makedirs(output_folder, exist_ok=True)

# 目標白色（純白）
target_white = np.array([255, 255, 255], dtype=np.float32)

# 讀取檔案清單
files = [f for f in os.listdir(input_folder) if f.lower().endswith((".jpg", ".png", ".jpeg"))]

print(f"總共找到 {len(files)} 張照片，開始處理...")

# 批量處理
for idx, filename in enumerate(files, 1):
    path = os.path.join(input_folder, filename)
    img = cv2.imread(path)
    img_rgb = cv2.cvtColor(img, cv2.COLOR_BGR2RGB).astype(np.float32)

    # 找白紙區
    brightness = np.mean(img_rgb, axis=2)
    mask = brightness > 200
    if np.sum(mask) == 0:
        mask = brightness > 180
    white_avg = np.mean(img_rgb[mask], axis=0)

    # 計算增益
    gain = target_white / (white_avg + 1e-5)
    corrected = img_rgb * gain
    corrected = np.clip(corrected, 0, 255).astype(np.uint8)

    # 儲存
    corrected_img = Image.fromarray(corrected)
    corrected_img.save(os.path.join(output_folder, filename))

    # 印出目前狀態
    print(f"[{idx}/{len(files)}] {filename} -> 白紙平均值: {white_avg.round(1)}, 增益: {gain.round(2)}")

print("批量白平衡校正完成！")
