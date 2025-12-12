import numpy as np
import json
import os
import sys
import time
import csv
import glob

class ArcSARMonitor:
    def __init__(self, config_path):
        print("[Init] 啟動 ArcSARMonitor 建構式")
        print(f"[Init] 讀取設定檔路徑 = {config_path}")

        # 1. 讀取設定
        try:
            with open(config_path, 'r', encoding='utf-8') as f:
                self.config = json.load(f)
            print("[Init] 設定檔讀取成功")
        except Exception as e:
            print(f"[Error] 無法讀取設定檔: {e}")
            sys.exit(1)
        
        print("[Init] 載入 monitor_indices, threshold, coherence_threshold")
        self.indices = self.config['monitor_indices']
        self.threshold = self.config['threshold_mm']
        self.coh_thr = self.config.get('coherence_threshold', 0.3)

        print("[Init] 設置 log 目錄")
        self.log_dir = "logs"
        if not os.path.exists(self.log_dir):
            os.makedirs(self.log_dir)
            print("[Init] 已建立 logs 目錄")
        else:
            print("[Init] logs 目錄已存在")

        # 2. 頻率與轉換因子
        print("[Init] 讀取頻率相關參數 fc_start 與 bw")
        try:
            fc_val = self.config['fc_start']
            bw_val = self.config['bw']
        except KeyError:
            print("[Error] Config 缺少 fc_start 或 bw 參數")
            sys.exit(1)

        c = 299792458
        f_center = fc_val + (bw_val / 2)
        self.factor = (-c * 1000) / (4 * np.pi * f_center)
        print(f"[Init] 轉換因子 factor = {self.factor}")

        print(f"[{time.strftime('%H:%M:%S')}] 系統初始化完成 (模式: 5分鐘間隔自動過濾)")

    # ------------------------------------------------------
    def read_binary_file(self, filepath, dtype=np.float32):
        print(f"[read_binary_file] 嘗試讀取二進位檔案: {filepath}")
        if not os.path.exists(filepath):
            print("[read_binary_file] 檔案不存在")
            return None

        try:
            data = np.fromfile(filepath, dtype=dtype)
            print(f"[read_binary_file] 讀取成功，資料長度 = {len(data)}")
            return data
        except Exception as e:
            print(f"[Error][read_binary_file] 讀取失敗: {e}")
            return None

    # ------------------------------------------------------
    def save_to_csv(self, filename, max_val, avg_val, is_alarm):
        print("[save_to_csv] 準備寫入 CSV 報表")
        current_month = time.strftime("%Y-%m")
        log_path = os.path.join(self.log_dir, f"{current_month}_Report.csv")
        file_exists = os.path.isfile(log_path)
        timestamp = time.strftime("%Y-%m-%d %H:%M:%S")
        status = "ALARM" if is_alarm else "NORMAL"

        print(f"[save_to_csv] 目標檔案 = {log_path}")
        print(f"[save_to_csv] 狀態 = {status}, Max={max_val}, Avg={avg_val}")

        try:
            with open(log_path, 'a', newline='', encoding='utf-8') as f:
                writer = csv.writer(f)
                if not file_exists:
                    print("[save_to_csv] 建立新 CSV，寫入表頭")
                    writer.writerow(["Timestamp", "Status", "Filename", "Max_Rebound_mm", "Avg_Rebound_mm", "Threshold_mm"])
                writer.writerow([timestamp, status, filename, f"{max_val:.4f}", f"{avg_val:.4f}", self.threshold])
            print(f"[save_to_csv] 已寫入 -> {current_month}_Report.csv")
        except Exception as e:
            print(f"[Error] CSV 寫入失敗: {e}")

    # ------------------------------------------------------
    def is_scan_2(self, current_file_path):
        print(f"[is_scan_2] 檢查是否為 Scan 2: {current_file_path}")

        folder_path = os.path.dirname(os.path.abspath(current_file_path))
        print(f"[is_scan_2] 目標資料夾 = {folder_path}")

        all_files = glob.glob(os.path.join(folder_path, "*.dphase"))
        print(f"[is_scan_2] 找到 {len(all_files)} 個 .dphase 檔案")

        all_files.sort(key=os.path.getmtime)
        print("[is_scan_2] 已依修改時間排序")

        try:
            abs_current = os.path.abspath(current_file_path)
            all_files_abs = [os.path.abspath(f) for f in all_files]

            curr_index = all_files_abs.index(abs_current)
            print(f"[is_scan_2] 當前檔案排序位置 index = {curr_index}")
        except ValueError:
            print("[is_scan_2] 找不到該檔案於列表中 -> 視為失敗")
            return False

        if curr_index == 0:
            print("[is_scan_2] 這是最舊檔案 -> 一定是 Scan 1 -> 忽略")
            return False

        prev_file = all_files[curr_index - 1]
        print(f"[is_scan_2] 前一檔案 = {prev_file}")

        curr_time = os.path.getmtime(current_file_path)
        prev_time = os.path.getmtime(prev_file)
        time_diff = curr_time - prev_time
        print(f"[is_scan_2] 時間差 = {time_diff:.1f} 秒")

        if 200 < time_diff < 420:
            print("[is_scan_2] 時間差符合 3~7 分鐘 -> 判定為 Scan 2")
            return True
        else:
            print("[is_scan_2] 時間差不符 -> 視為 Scan 1 或其他 -> 忽略")
            return False

    # ------------------------------------------------------
    def process_rebound(self, diff_dphase_path):
        print(f"[process_rebound] 開始處理檔案: {diff_dphase_path}")

        # 檢查是不是 Scan 2
        if not self.is_scan_2(diff_dphase_path):
            print("[process_rebound] 不是 Scan 2 -> 不計算")
            return False

        print("[process_rebound] 是 Scan 2 -> 開始讀取資料")
        dphi_data = self.read_binary_file(diff_dphase_path)
        if dphi_data is None:
            print("[process_rebound] 無法讀取資料 -> 結束")
            return False

        try:
            max_idx = len(dphi_data)
            print(f"[process_rebound] 資料長度 = {max_idx}")
            print(f"[process_rebound] 監控 index = {self.indices}")

            valid_indices = [i for i in self.indices if i < max_idx]
            print(f"[process_rebound] 有效 index = {valid_indices}")

            if not valid_indices:
                print("[process_rebound] 有效 index 為空 -> 結束")
                return False

            displacements = dphi_data[valid_indices] * self.factor
            max_rebound = np.max(np.abs(displacements))
            avg_rebound = np.mean(np.abs(displacements))
            
            print(f"[process_rebound] Max={max_rebound:.3f}mm, Avg={avg_rebound:.3f}mm")

            is_alarm = max_rebound > self.threshold
            print(f"[process_rebound] 是否警報?: {is_alarm}")

            self.save_to_csv(os.path.basename(diff_dphase_path), max_rebound, avg_rebound, is_alarm)
            print("[process_rebound] 計算完成")
            return is_alarm

        except Exception as e:
            print(f"[Error][process_rebound] 計算錯誤: {e}")
            return False


# ------------------------------------------------------
if __name__ == "__main__":
    print("[Main] 程式啟動，參數如下：")
    print(f"[Main] sys.argv = {sys.argv}")

    if len(sys.argv) < 3:
        print("[Main] 參數不足 -> 結束 (視為正常)")
        sys.exit(0)

    config_path = sys.argv[1]
    target_file = sys.argv[2]

    print(f"[Main] config_path = {config_path}")
    print(f"[Main] target_file = {target_file}")

    monitor = ArcSARMonitor(config_path)
    alarm = monitor.process_rebound(target_file)

    print(f"[Main] 最終 alarm = {alarm}")

    if alarm:
        print("[Main] !!! 警報 !!!")
        sys.exit(1)
    else:
        sys.exit(0)
