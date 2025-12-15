import numpy as np
import json
import os
import time
import csv
import glob
import sys
import socket

# =============================
# 參數設定區 (使用者可調整)
# =============================

# 1. 程式執行頻率
CHECK_INTERVAL = 1      # 每隔幾秒檢查一次 (單位：秒)

# 2. 影像判斷邏輯 (Scan 2 判定)
SCAN_TIME_MIN = 30       # 最小時間差 (秒)
SCAN_TIME_MAX = 420      # 最大時間差 (秒)

# 3. 路徑設定
try:
    BASE_DIR = os.path.dirname(os.path.abspath(__file__))
except NameError:
    BASE_DIR = os.path.abspath(os.getcwd())

CONFIG_PATH = os.path.join(BASE_DIR, "config.json")
LOG_DIR = os.path.join(BASE_DIR, "logs")

# 4. 資料來源 (自動判斷)
hostname = socket.gethostname()
if "DELL" in hostname:
    DATA_DIR = r"C:\Users\b5566\Desktop\ftp_download\cache"
else:
    DATA_DIR = r"C:\Users\b5566\Desktop\ftp_download\cache"

print(f"==========================================")
print(f"[設定] 檢查間隔: {CHECK_INTERVAL} 秒")
print(f"[設定] Scan判定: {SCAN_TIME_MIN} ~ {SCAN_TIME_MAX} 秒")
print(f"[設定] 資料來源: {DATA_DIR}")
print(f"==========================================\n")


# =============================
# 核心類別
# =============================

class ArcSARMonitor:
    def __init__(self, config_path, log_dir, data_dir, time_min, time_max):
        # 接收外部傳入的時間參數
        self.log_dir = log_dir
        self.data_dir = data_dir
        self.scan_time_min = time_min
        self.scan_time_max = time_max
        
        if not os.path.exists(self.log_dir):
            os.makedirs(self.log_dir)

        # 1. 讀取設定檔
        try:
            with open(config_path, 'r', encoding='utf-8') as f:
                self.config = json.load(f)
        except Exception as e:
            print(f"[Error] 讀取 config 失敗: {e}")
            sys.exit(1)

        self.indices = self.config['monitor_indices']
        self.threshold = self.config['threshold_mm']
        
        # 2. 自動載入座標參數
        self.coords_map = None  
        self.load_geometry_info()

        # 3. 計算相位轉位移的因子
        fc_val = self.config['fc_start']
        bw_val = self.config['bw']
        c = 299792458
        f_center = fc_val + bw_val / 2
        self.factor = (-c * 1000) / (4 * np.pi * f_center)

    def load_geometry_info(self):
        """ 讀取 *.image.txt 並建立座標對照表 """
        image_txt_files = glob.glob(os.path.join(self.data_dir, "*.image.txt"))
        
        if not image_txt_files:
            print("[Geometry] ⚠️ 找不到 *.image.txt，僅顯示 Index。")
            return

        param_file = sorted(image_txt_files)[0]
        params = {}
        try:
            with open(param_file, 'r', encoding='unicode_escape') as f:
                for line in f:
                    if '=' in line:
                        k, v = line.split('=', 1)
                        params[k.strip()] = v.strip()
        except:
            pass

        # 嘗試找 Nx, Ny 參數檔
        nx_ny_file = param_file.replace(".image.txt", "").replace(".image", "")
        target_path = nx_ny_file if os.path.exists(nx_ny_file) else param_file
        
        try:
            with open(target_path, 'r', encoding='unicode_escape') as f:
                for line in f:
                    if '=' in line:
                        k, v = line.split('=', 1)
                        params[k.strip()] = v.strip()
        except:
            pass 

        try:
            dx = float(params.get('dx', 0.3))
            dy = float(params.get('dy', 0.3))
            nx = int(params.get('Nx', 134))
            ny = int(params.get('Ny', 134))
            
            grid_x, grid_r = np.mgrid[1:nx+1, 1:ny+1]
            real_x = dx * (grid_x.ravel() - ((nx + 1) / 2))
            r_flat = grid_r.ravel()[::-1] 
            real_r = dy * (r_flat - ((ny + 1) / 2))
            self.coords_map = np.column_stack((real_x, real_r))
            print("[Geometry] 座標對照表建立完成")

        except Exception as e:
            print(f"[Error] 座標計算失敗: {e}")
            self.coords_map = None

    def get_real_coord_str(self, idx):
        if self.coords_map is None:
            return "N/A"
        if idx < len(self.coords_map):
            return f"(X:{self.coords_map[idx][0]:.2f}m, R:{self.coords_map[idx][1]:.2f}m)"
        return "Idx_Out"

    def read_binary_file(self, filepath, dtype=np.float32):
        if not os.path.exists(filepath):
            return None
        try:
            return np.fromfile(filepath, dtype=dtype)
        except:
            return None

    def is_scan_2(self, current_file_path):
        """ 判斷是否為穩定的 Scan 2 """
        folder_path = os.path.dirname(os.path.abspath(current_file_path))
        all_files = glob.glob(os.path.join(folder_path, "*.dphase"))
        all_files.sort(key=os.path.getmtime)
        abs_current = os.path.abspath(current_file_path)
        
        try:
            curr_index = -1
            for i, f in enumerate(all_files):
                if os.path.abspath(f) == abs_current:
                    curr_index = i
                    break
        except:
            return False

        if curr_index <= 0:
            return False

        prev_file = all_files[curr_index - 1]
        curr_time = os.path.getmtime(current_file_path)
        prev_time = os.path.getmtime(prev_file)
        time_diff = curr_time - prev_time
        
        # 使用全域變數判斷
        return self.scan_time_min < time_diff < self.scan_time_max

    def save_to_csv(self, filename, max_val, avg_val, is_alarm, top_3_details):
        current_month = time.strftime("%Y-%m")
        log_path = os.path.join(self.log_dir, f"{current_month}_Report.csv")
        file_exists = os.path.isfile(log_path)
        timestamp = time.strftime("%Y-%m-%d %H:%M:%S")
        status = "ALARM" if is_alarm else "NORMAL"
        
        try:
            with open(log_path, 'a', newline='', encoding='utf-8') as f:
                writer = csv.writer(f)
                if not file_exists:
                    writer.writerow(["Timestamp", "Status", "Filename", "Max_Rebound_mm", "Avg_Rebound_mm", "Threshold_mm", "Top3_Details"])
                writer.writerow([timestamp, status, filename, f"{max_val:.4f}", f"{avg_val:.4f}", self.threshold, top_3_details])
            print(f"[Log] 已寫入: {os.path.basename(log_path)}")
        except Exception as e:
            print(f"[Error] CSV 寫入失敗: {e}")

    def trigger_alarm(self, top_3_details):
        """ 
        警報觸發函式 
        目前功能：僅在 Console 印出明顯標示，不發出聲音。
        未來功能：可在此加入 Telegram / Line 推播。
        """
        print("\n" + "="*30)
        print("   ⚠️  警報觸發 (Alarm Triggered)  ⚠️")
        print("="*30)
        # 您可以在這裡加入 Telegram 發送邏輯

    def process_rebound(self, diff_dphase_path):
        print(f"\n[Process] 處理檔案: {os.path.basename(diff_dphase_path)}")
        
        if not self.is_scan_2(diff_dphase_path):
            print(f"[Process] 時間間隔不符 ({self.scan_time_min}~{self.scan_time_max}s) -> 略過")
            return False

        dphi_data = self.read_binary_file(diff_dphase_path)
        if dphi_data is None: return False

        valid_indices = [i for i in self.indices if i < len(dphi_data)]
        if not valid_indices: return False
        
        valid_indices_arr = np.array(valid_indices)
        displacements = dphi_data[valid_indices_arr] * self.factor
        abs_displacements = np.abs(displacements)

        max_rebound = np.max(abs_displacements)
        avg_rebound = np.mean(abs_displacements)
        is_alarm = max_rebound > self.threshold

        top_3_str = "None"
        if is_alarm:
            alarm_mask = abs_displacements > self.threshold
            alarm_vals = abs_displacements[alarm_mask]
            alarm_idxs = valid_indices_arr[alarm_mask]
            sort_order = np.argsort(alarm_vals)[::-1]
            
            details_list = []
            print(f"[!!! 警報 !!!] 超標點數: {len(alarm_vals)}")
            for k in range(min(3, len(alarm_vals))):
                idx = alarm_idxs[sort_order][k]
                val = alarm_vals[sort_order][k]
                coord = self.get_real_coord_str(idx)
                details_list.append(f"ID:{idx} Val:{val:.2f}mm Loc:{coord}")
                print(f"  -> Index {idx}: {val:.2f}mm {coord}")
            
            top_3_str = " | ".join(details_list)
            
            # 呼叫警報函式 (這裡已經拿掉聲音了)
            self.trigger_alarm(top_3_str)

        print(f"[Result] Max={max_rebound:.3f}mm, Alarm={is_alarm}")
        self.save_to_csv(os.path.basename(diff_dphase_path), max_rebound, avg_rebound, is_alarm, top_3_str)
        return is_alarm

# =============================
# 主程式 (迴圈監測版)
# =============================
def main():
    print(f"[Main] 啟動自動監測循環 (間隔 {CHECK_INTERVAL} 秒)...")
    print(f"[Main] 按下 Ctrl + C 可停止程式")
    
    # 初始化監測器
    monitor = ArcSARMonitor(CONFIG_PATH, LOG_DIR, DATA_DIR, SCAN_TIME_MIN, SCAN_TIME_MAX)
    
    last_processed_file = None

    try:
        while True:
            # 1. 找最新檔案
            files = glob.glob(os.path.join(DATA_DIR, "*.dphase"))
            
            if files:
                latest_file = max(files, key=os.path.getmtime)
                
                # 2. 判斷是否已經處理過
                if latest_file != last_processed_file:                    
                    print(f"\n[Main] {time.strftime('%H:%M:%S')} 發現新檔案: {latest_file}")
                    monitor.process_rebound(latest_file)
                    last_processed_file = latest_file
                else:
                    # 檔案沒變，休息
                    print(f"\r[Main] {time.strftime('%H:%M:%S')} 無新檔案，等待中...", end="", flush=True)
                    pass
            else:
                print("[Main] 目前資料夾內無 .dphase 檔案")

            # 3. 休息指定秒數
            time.sleep(CHECK_INTERVAL)

    except KeyboardInterrupt:
        print("\n[Main] 使用者手動停止程式。")
        sys.exit(0)
    except Exception as e:
        print(f"\n[Main] 發生未預期錯誤: {e}")
        sys.exit(1)

if __name__ == "__main__":
    main()