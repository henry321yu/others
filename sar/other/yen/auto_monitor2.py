import numpy as np
import json
import os
import time
import csv
import glob
import sys

# =============================
# 使用者設定
CONFIG = r"C:\Users\b5566\Desktop\hey\git\others\sar\other\yen\config.json"
DATA_DIR = r"C:\Users\b5566\Desktop\ftp_download\20250908_TS_test5\cache"
LOG_DIR = r"C:\Users\b5566\Desktop\hey\git\others\sar\other\yen\logs"
# =============================

class ArcSARMonitor:
    def __init__(self, config_path, log_dir):
        print("[Init] 啟動 ArcSARMonitor")
        self.log_dir = log_dir
        if not os.path.exists(self.log_dir):
            os.makedirs(self.log_dir)
            print(f"[Init] 已建立 log 目錄: {self.log_dir}")
        else:
            print(f"[Init] log 目錄已存在: {self.log_dir}")

        # 讀取設定
        try:
            with open(config_path, 'r', encoding='utf-8') as f:
                self.config = json.load(f)
            print(f"[Init] 成功讀取設定檔: {config_path}")
        except Exception as e:
            print(f"[Error] 讀取 config 失敗: {e}")
            sys.exit(1)

        self.indices = self.config['monitor_indices']
        self.threshold = self.config['threshold_mm']
        self.coh_thr = self.config.get('coherence_threshold', 0.3)

        fc_val = self.config['fc_start']
        bw_val = self.config['bw']
        c = 299792458
        f_center = fc_val + bw_val / 2
        self.factor = (-c * 1000) / (4 * np.pi * f_center)
        print(f"[Init] 轉換因子 factor = {self.factor}")

    def read_binary_file(self, filepath, dtype=np.float32):
        print(f"[read_binary_file] 嘗試讀取: {filepath}")
        if not os.path.exists(filepath):
            print("[read_binary_file] 檔案不存在")
            return None
        try:
            data = np.fromfile(filepath, dtype=dtype)
            print(f"[read_binary_file] 讀取成功，資料長度 = {len(data)}")
            return data
        except Exception as e:
            print(f"[Error] 讀取二進位檔失敗: {e}")
            return None

    def save_to_csv(self, filename, max_val, avg_val, is_alarm):
        current_month = time.strftime("%Y-%m")
        log_path = os.path.join(self.log_dir, f"{current_month}_Report.csv")
        file_exists = os.path.isfile(log_path)
        timestamp = time.strftime("%Y-%m-%d %H:%M:%S")
        status = "ALARM" if is_alarm else "NORMAL"
        try:
            with open(log_path, 'a', newline='', encoding='utf-8') as f:
                writer = csv.writer(f)
                if not file_exists:
                    writer.writerow(["Timestamp", "Status", "Filename", "Max_Rebound_mm", "Avg_Rebound_mm", "Threshold_mm"])
                writer.writerow([timestamp, status, filename, f"{max_val:.4f}", f"{avg_val:.4f}", self.threshold])
            print(f"[save_to_csv] 已寫入 {log_path}")
        except Exception as e:
            print(f"[Error] CSV 寫入失敗: {e}")

    def is_scan_2(self, current_file_path):
        folder_path = os.path.dirname(os.path.abspath(current_file_path))
        all_files = glob.glob(os.path.join(folder_path, "*.dphase"))
        all_files.sort(key=os.path.getmtime)
        abs_current = os.path.abspath(current_file_path)
        all_files_abs = [os.path.abspath(f) for f in all_files]

        try:
            curr_index = all_files_abs.index(abs_current)
        except ValueError:
            print("[is_scan_2] 找不到檔案於列表中")
            return False

        if curr_index == 0:
            print("[is_scan_2] 最舊檔案 -> Scan 1")
            return False

        prev_file = all_files[curr_index - 1]
        curr_time = os.path.getmtime(current_file_path)
        prev_time = os.path.getmtime(prev_file)
        time_diff = curr_time - prev_time
        print(f"[is_scan_2] 與前一檔案時間差: {time_diff:.1f} 秒")
        return 200 < time_diff < 420

    def process_rebound(self, diff_dphase_path):
        print(f"[process_rebound] 開始處理: {diff_dphase_path}")
        if not self.is_scan_2(diff_dphase_path):
            print("[process_rebound] 不是 Scan 2 -> 不計算")
            return False

        dphi_data = self.read_binary_file(diff_dphase_path)
        if dphi_data is None:
            return False

        valid_indices = [i for i in self.indices if i < len(dphi_data)]
        if not valid_indices:
            print("[process_rebound] 有效 index 為空 -> 結束")
            return False

        displacements = dphi_data[valid_indices] * self.factor
        max_rebound = np.max(np.abs(displacements))
        avg_rebound = np.mean(np.abs(displacements))
        is_alarm = max_rebound > self.threshold

        print(f"[process_rebound] Max={max_rebound:.3f}mm, Avg={avg_rebound:.3f}mm, Alarm={is_alarm}")
        self.save_to_csv(os.path.basename(diff_dphase_path), max_rebound, avg_rebound, is_alarm)
        return is_alarm

# =============================
# 主程式
def main():
    print("[Main] 自動抓取最新 .dphase 檔案")
    files = glob.glob(os.path.join(DATA_DIR, "*.dphase"))
    if not files:
        print("[Main] 沒有找到任何 .dphase 檔案，結束")
        sys.exit(0)

    latest_file = max(files, key=os.path.getmtime)
    print(f"[Main] 最新檔案: {latest_file}")

    monitor = ArcSARMonitor(CONFIG, LOG_DIR)
    alarm = monitor.process_rebound(latest_file)

    if alarm:
        print("[Main] !!! 警報 !!!")
        sys.exit(1)
    else:
        print("[Main] 正常，無警報")
        sys.exit(0)

if __name__ == "__main__":
    main()
