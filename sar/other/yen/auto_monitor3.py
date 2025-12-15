import numpy as np
import json
import os
import time
import csv
import glob
import sys
import socket

# =============================
# 全域設定與路徑初始化
# =============================

# 1. 取得這支程式所在的資料夾 (基地路徑)
try:
    # 嘗試取得 __file__ (適用於 .py 腳本)
    BASE_DIR = os.path.dirname(os.path.abspath(__file__))
except NameError:
    # 如果 __file__ 未定義 (適用於 Jupyter Notebook)
    BASE_DIR = os.path.abspath(os.getcwd())

# 2. 設定 Config 與 Log 路徑
CONFIG_PATH = os.path.join(BASE_DIR, "config.json")
LOG_DIR = os.path.join(BASE_DIR, "logs")

# 3. 自動切換資料來源路徑 (依據電腦名稱)
hostname = socket.gethostname()
if "DELL" in hostname:  # 假設 DELL 電腦
    DATA_DIR = r"C:\Users\b5566\Desktop\ftp_download\cache"
else:
    # 測試用路徑
    DATA_DIR = r"C:\Users\b5566\Desktop\ftp_download\cache"

print(f"[Info] 當前電腦: {hostname}")
print(f"[Info] 程式基地: {BASE_DIR}")
print(f"[Info] 資料來源: {DATA_DIR}")

# =============================
# 核心類別
# =============================

class ArcSARMonitor:
    def __init__(self, config_path, log_dir, data_dir):
        print("\n[Init] 啟動 ArcSARMonitor V2.0 (含座標自動解析)")
        self.log_dir = log_dir
        self.data_dir = data_dir
        
        if not os.path.exists(self.log_dir):
            os.makedirs(self.log_dir)
            print(f"[Init] 已建立 log 目錄: {self.log_dir}")

        # 1. 讀取設定檔
        try:
            with open(config_path, 'r', encoding='utf-8') as f:
                self.config = json.load(f)
            print(f"[Init] 成功讀取設定檔: {config_path}")
        except Exception as e:
            print(f"[Error] 讀取 config 失敗: {e}")
            sys.exit(1)

        self.indices = self.config['monitor_indices']
        self.threshold = self.config['threshold_mm']
        
        # 2. 自動載入座標參數 (取代手動設定 image_width)
        # self.coords_map 將儲存每個 pixel 的 (X, R) 真實座標
        self.coords_map = None  
        self.load_geometry_info()

        # 3. 計算相位轉位移的因子
        fc_val = self.config['fc_start']
        bw_val = self.config['bw']
        c = 299792458
        f_center = fc_val + bw_val / 2
        self.factor = (-c * 1000) / (4 * np.pi * f_center)
        print(f"[Init] 轉換因子 factor = {self.factor:.6f}")

    def load_geometry_info(self):
        """
        移植自舊程式碼：讀取 *.image.txt 並建立座標對照表
        """
        print("[Geometry] 正在搜尋影像參數檔 (*.image.txt)...")
        # 搜尋資料夾下的 .image.txt 檔案
        image_txt_files = glob.glob(os.path.join(self.data_dir, "*.image.txt"))
        
        if not image_txt_files:
            print("[Warning] 找不到 *.image.txt，將無法計算真實座標，僅能顯示 Index。")
            return

        # 依照舊程式邏輯，取排序後的第一個
        param_file = sorted(image_txt_files)[0]
        print(f"[Geometry] 讀取參數檔: {os.path.basename(param_file)}")

        params = {}
        # A. 讀取主要參數 (嘗試讀取 dx, dy)
        try:
            with open(param_file, 'r', encoding='unicode_escape') as f:
                for line in f:
                    if '=' in line:
                        k, v = line.split('=', 1)
                        params[k.strip()] = v.strip()
        except Exception as e:
            print(f"[Error] 解析參數檔失敗: {e}")
            return

        # B. 讀取 Nx, Ny
        # 舊程式邏輯是找同檔名但沒有副類型的檔案，這裡做個容錯
        # 嘗試找去掉 .image.txt 後的檔名
        nx_ny_file = param_file.replace(".image.txt", "").replace(".image", "")
        
        # 如果該檔案不存在，或是跟原本一樣，就嘗試讀原本那個
        target_nxny_path = nx_ny_file if os.path.exists(nx_ny_file) else param_file
        
        try:
            with open(target_nxny_path, 'r', encoding='unicode_escape') as f:
                for line in f:
                    if '=' in line:
                        k, v = line.split('=', 1)
                        params[k.strip()] = v.strip()
        except:
            pass 

        # C. 建立座標網格
        try:
            # 使用 get 避免 key error，預設值設為安全值
            dx = float(params.get('dx', 0.3))
            dy = float(params.get('dy', 0.3))
            nx = int(params.get('Nx', 134))
            ny = int(params.get('Ny', 134))
            
            print(f"[Geometry] 參數解析成功: Nx={nx}, Ny={ny}, dx={dx}, dy={dy}")
            
            # D. 重現舊程式的座標計算邏輯
            # 產生網格: 1 到 Nx, 1 到 Ny
            grid_x, grid_r = np.mgrid[1:nx+1, 1:ny+1]
            
            # 計算 X (Cross-Range)
            # 舊公式: dx * (X_range - (Nx+1)/2)
            real_x = dx * (grid_x.ravel() - ((nx + 1) / 2))
            
            # 計算 R (Range) - 包含反轉 [::-1]
            # 舊公式: dy * (R_range[::-1] - (Ny+1)/2)
            # 注意：ravel() 後是扁平的，我們對其做反轉以符合舊程式邏輯
            r_flat = grid_r.ravel()[::-1] 
            real_r = dy * (r_flat - ((ny + 1) / 2))
            
            # 組合起來: 變成一個大表，第 i 個 row 就是 index i 的 (X, R)
            # column_stack 會把兩個 1D array 併成一個 2D array (N, 2)
            self.coords_map = np.column_stack((real_x, real_r))
            
            print("[Geometry] 座標對照表建立完成！")

        except Exception as e:
            print(f"[Error] 計算座標網格失敗: {e}")
            self.coords_map = None

    def get_real_coord_str(self, idx):
        """ 傳入 index，回傳格式化的真實座標字串 """
        if self.coords_map is None:
            return "N/A"
        
        if idx < len(self.coords_map):
            x_val = self.coords_map[idx][0]
            r_val = self.coords_map[idx][1]
            # 回傳格式 (X: 10.5m, R: -5.2m)
            return f"(X:{x_val:.2f}m, R:{r_val:.2f}m)"
        else:
            return "Idx_Out"

    def read_binary_file(self, filepath, dtype=np.float32):
        if not os.path.exists(filepath):
            return None
        try:
            data = np.fromfile(filepath, dtype=dtype)
            return data
        except Exception as e:
            print(f"[Error] 讀取二進位檔失敗: {e}")
            return None

    def is_scan_2(self, current_file_path):
        """ 判斷是否為穩定的 Scan 2 (比較時間差) """
        folder_path = os.path.dirname(os.path.abspath(current_file_path))
        all_files = glob.glob(os.path.join(folder_path, "*.dphase"))
        all_files.sort(key=os.path.getmtime)
        abs_current = os.path.abspath(current_file_path)
        all_files_abs = [os.path.abspath(f) for f in all_files]

        try:
            curr_index = all_files_abs.index(abs_current)
        except ValueError:
            return False

        if curr_index == 0:
            return False

        prev_file = all_files[curr_index - 1]
        curr_time = os.path.getmtime(current_file_path)
        prev_time = os.path.getmtime(prev_file)
        time_diff = curr_time - prev_time
        
        # 時間差在 30 ~ 420 秒之間才算有效
        return 30 < time_diff < 420

    def save_to_csv(self, filename, max_val, avg_val, is_alarm, top_3_details):
        """ 寫入 CSV Log """
        current_month = time.strftime("%Y-%m")
        log_path = os.path.join(self.log_dir, f"{current_month}_Report.csv")
        file_exists = os.path.isfile(log_path)
        timestamp = time.strftime("%Y-%m-%d %H:%M:%S")
        status = "ALARM" if is_alarm else "NORMAL"
        
        try:
            with open(log_path, 'a', newline='', encoding='utf-8') as f:
                writer = csv.writer(f)
                # 寫入 Header
                if not file_exists:
                    writer.writerow([
                        "Timestamp", "Status", "Filename", 
                        "Max_Rebound_mm", "Avg_Rebound_mm", "Threshold_mm", 
                        "Top3_Details (Real Coords)"
                    ])
                
                # 寫入資料
                writer.writerow([
                    timestamp, status, filename, 
                    f"{max_val:.4f}", f"{avg_val:.4f}", self.threshold, 
                    top_3_details
                ])
            print(f"[Log] 已寫入報表: {log_path}")
        except Exception as e:
            print(f"[Error] CSV 寫入失敗: {e}")

    def process_rebound(self, diff_dphase_path):
        print(f"\n[Process] 開始處理: {os.path.basename(diff_dphase_path)}")
        
        # 1. 檢查是否為有效 Scan
        if not self.is_scan_2(diff_dphase_path):
            print("[Process] 不是 Scan 2 或時間間隔不對 -> 略過")
            return False

        # 2. 讀取資料
        dphi_data = self.read_binary_file(diff_dphase_path)
        if dphi_data is None:
            return False

        # 3. 過濾有效的 Indices (邊界檢查)
        valid_indices = []
        for i in self.indices:
            if i < len(dphi_data):
                valid_indices.append(i)
            else:
                print(f"[Warning] Index {i} 超出影像範圍 (Size: {len(dphi_data)})")
        
        if not valid_indices:
            print("[Process] 沒有有效的 Index -> 結束")
            return False
        
        valid_indices_arr = np.array(valid_indices)
        
        # 4. 計算位移 (mm)
        displacements = dphi_data[valid_indices_arr] * self.factor
        abs_displacements = np.abs(displacements)

        # 5. 統計數據
        max_rebound = np.max(abs_displacements)
        avg_rebound = np.mean(abs_displacements)
        is_alarm = max_rebound > self.threshold

        # 6. 處理詳細資訊 (前三名)
        top_3_str = "None"
        if is_alarm:
            # 找出所有超標點
            alarm_mask = abs_displacements > self.threshold
            alarm_vals = abs_displacements[alarm_mask]
            alarm_idxs = valid_indices_arr[alarm_mask]
            
            # 排序 (由大到小)
            sort_order = np.argsort(alarm_vals)[::-1]
            sorted_vals = alarm_vals[sort_order]
            sorted_idxs = alarm_idxs[sort_order]
            
            details_list = []
            print(f"[!!! 警報 !!!] 共 {len(sorted_vals)} 個點超標")
            
            # 取前 3 名
            count = min(3, len(sorted_vals))
            for k in range(count):
                idx = sorted_idxs[k]
                val = sorted_vals[k]
                
                # 取得真實座標
                coord_str = self.get_real_coord_str(idx)
                
                # 格式化字串
                info = f"ID:{idx} Val:{val:.2f}mm Loc:{coord_str}"
                details_list.append(info)
                
                print(f"  -> 第 {k+1} 名: Index {idx}, 沉陷 {val:.4f} mm, 座標 {coord_str}")

            top_3_str = " | ".join(details_list)

        print(f"[Result] Max={max_rebound:.3f}mm, Alarm={is_alarm}")
        
        # 7. 寫入 Log
        self.save_to_csv(os.path.basename(diff_dphase_path), max_rebound, avg_rebound, is_alarm, top_3_str)
        
        return is_alarm

# =============================
# 主程式進入點
# =============================
def main():
    print("==========================================")
    print("   ArcSAR 自動監測系統 (常駐模式)")
    print("==========================================")

    # 初始化監測器（只做一次）
    monitor = ArcSARMonitor(CONFIG_PATH, LOG_DIR, DATA_DIR)

    last_processed_file = None

    try:
        while True:
            # 尋找所有 .dphase
            files = glob.glob(os.path.join(DATA_DIR, "*.dphase"))
            if not files:
                print("[Main] 尚無 .dphase 檔案，5 秒後再檢查")
                time.sleep(5)
                continue

            # 取得最新檔案
            latest_file = max(files, key=os.path.getmtime)

            # 若與上次相同，代表沒有新資料
            if latest_file == last_processed_file:
                print("[Main] 無新檔案，等待中...")
            else:
                print(f"\n[Main] 發現新檔案: {latest_file}")
                alarm = monitor.process_rebound(latest_file)

                if alarm:
                    print("[Main] !!! 警報狀態（程式持續運行）!!!")

                last_processed_file = latest_file

            # 每 5 秒檢查一次
            time.sleep(5)

    except KeyboardInterrupt:
        print("\n[Main] 使用者中斷程式 (Ctrl+C)，安全結束。")


if __name__ == "__main__":
    main()