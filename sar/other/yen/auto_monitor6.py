import numpy as np
import json
import os
import time
import csv
import glob
import sys
import socket
import requests

# =============================
# 參數設定區
# =============================

# 1. 程式執行頻率
CHECK_INTERVAL = 1      # 每隔幾秒檢查一次

# 2. 影像判斷邏輯
SCAN_TIME_MIN = 30       
SCAN_TIME_MAX = 420      

# 3. 路徑設定
try:
    BASE_DIR = os.path.dirname(os.path.abspath(__file__))
except NameError:
    BASE_DIR = os.path.abspath(os.getcwd())

CONFIG_PATH = os.path.join(BASE_DIR, "config.json")
LOG_DIR = os.path.join(BASE_DIR, "logs")

# 4. 資料來源
hostname = socket.gethostname()
if "DELL" in hostname:
    DATA_DIR = r"C:\Users\b5566\Desktop\ftp_download\cache"
else:
    DATA_DIR = r"C:\Users\b5566\Desktop\ftp_download\cache"

# 5. Discord Webhook
DISCORD_WEBHOOK_URL = "https://discord.com/api/webhooks/1449977134989840405/ME2fc5_gNJOQXKv1HAhzo7z22LYNRygHV8iM3cpird5atljJ22GcnX4NNRQU5LC2-T-v"

print(f"==========================================")
print(f"[設定] 檢查間隔: {CHECK_INTERVAL} 秒")
print(f"[設定] 資料來源: {DATA_DIR}")
print(f"==========================================\n")


class ArcSARMonitor:
    def __init__(self, config_path, log_dir, data_dir, time_min, time_max):
        self.log_dir = log_dir
        self.data_dir = data_dir
        self.scan_time_min = time_min
        self.scan_time_max = time_max
        
        if not os.path.exists(self.log_dir):
            os.makedirs(self.log_dir)

        # 讀取設定
        try:
            with open(config_path, 'r', encoding='utf-8') as f:
                self.config = json.load(f)
        except Exception as e:
            print(f"[Error] 讀取 config 失敗: {e}")
            sys.exit(1)

        self.indices = self.config['monitor_indices']
        self.threshold = self.config['threshold_mm']
        
        # 載入座標
        self.coords_map = None  
        self.load_geometry_info()

        # 計算因子
        fc_val = self.config['fc_start']
        bw_val = self.config['bw']
        c = 299792458
        f_center = fc_val + bw_val / 2
        self.factor = (-c * 1000) / (4 * np.pi * f_center)

    def load_geometry_info(self):
        """ 讀取 *.image.txt 並建立座標對照表 """
        image_txt_files = glob.glob(os.path.join(self.data_dir, "*.image.txt"))
        if not image_txt_files:
            print("[Geometry] ⚠️ 找不到 *.image.txt")
            return

        param_file = sorted(image_txt_files)[0]
        params = {}
        
        def read_params(path):
            if os.path.exists(path):
                try:
                    with open(path, 'r', encoding='unicode_escape') as f:
                        for line in f:
                            if '=' in line:
                                k, v = line.split('=', 1)
                                params[k.strip()] = v.strip()
                except: pass

        read_params(param_file)
        nx_ny_file = param_file.replace(".image.txt", "").replace(".image", "")
        if nx_ny_file != param_file:
            read_params(nx_ny_file)

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
        if self.coords_map is None: return "N/A"
        if idx < len(self.coords_map):
            return f"(X:{self.coords_map[idx][0]:.1f}m, R:{self.coords_map[idx][1]:.1f}m)"
        return "Idx_Out"

    def read_binary_file(self, filepath, dtype=np.float32):
        if not os.path.exists(filepath): return None
        try: return np.fromfile(filepath, dtype=dtype)
        except: return None

    def is_scan_2(self, current_file_path):
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
        except: return False

        if curr_index <= 0: return False

        prev_file = all_files[curr_index - 1]
        time_diff = os.path.getmtime(current_file_path) - os.path.getmtime(prev_file)
        return self.scan_time_min < time_diff < self.scan_time_max

    def save_trend_csv(self, filename, valid_indices, displacements):
        """
        [新功能] 儲存所有監測點的位移歷程
        檔名範例: 2025-12_Trend_Matrix.csv
        """
        current_month = time.strftime("%Y-%m")
        trend_log_path = os.path.join(self.log_dir, f"{current_month}_Trend_Matrix.csv")
        file_exists = os.path.isfile(trend_log_path)
        timestamp = time.strftime("%Y-%m-%d %H:%M:%S")

        # 1. 建立這一輪的數據字典 (Key: Index, Value: Displacement)
        # 先將所有 config 裡的 index 預設為空值 (避免這張圖剛好該點超出範圍)
        row_data = {str(idx): "" for idx in self.indices}
        
        # 填入計算出的數值
        for idx, val in zip(valid_indices, displacements):
            # 取絕對值或原始值？通常做趨勢分析看絕對值比較直觀 (Rebound)，也可依需求改
            # 這裡使用絕對值，與告警邏輯一致
            row_data[str(idx)] = f"{abs(val):.4f}" 

        # 2. 準備 CSV 欄位名稱
        # 固定前兩欄 + Config 裡的所有 Index
        fieldnames = ["Timestamp", "Filename"] + [str(idx) for idx in self.indices]
        
        # 3. 寫入 CSV
        try:
            with open(trend_log_path, 'a', newline='', encoding='utf-8') as f:
                writer = csv.DictWriter(f, fieldnames=fieldnames)
                
                # 如果是新檔案，寫入 Header
                if not file_exists:
                    writer.writeheader()
                
                # 寫入資料列
                output_row = {"Timestamp": timestamp, "Filename": filename}
                output_row.update(row_data) # 合併位移數據
                
                writer.writerow(output_row)
                
            print(f"[Log] 趨勢紀錄已更新: {os.path.basename(trend_log_path)}")
        except Exception as e:
            print(f"[Error] 趨勢 CSV 寫入失敗: {e}")

    def save_to_csv(self, filename, max_val, avg_val, is_alarm, top_3_details):
        """ 原有的警報摘要 Log """
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
            print(f"[Log] 警報報表已寫入: {os.path.basename(log_path)}")
        except Exception as e:
            print(f"[Error] CSV 寫入失敗: {e}")

    def trigger_discord(self, filename, max_val, avg_val, is_alarm, top_3_details):
        if not DISCORD_WEBHOOK_URL: return

        color = 15158332 if is_alarm else 3066993
        title = "⚠️ ArcSAR Rebound Alarm" if is_alarm else "✅ ArcSAR Monitor Status"
        status_text = "ALARM" if is_alarm else "NORMAL"

        payload = {
            "username": "ArcSAR Monitor",
            "embeds": [
                {
                    "title": title,
                    "color": color,
                    "fields": [
                        {
                            "name": "時間",
                            "value": time.strftime("%Y-%m-%d %H:%M:%S"),
                            "inline": False
                        },
                        {
                            "name": "檔案",
                            "value": filename,
                            "inline": False
                        },
                        {
                            "name": "狀態",
                            "value": status_text,
                            "inline": True
                        },
                        {
                            "name": "Max / Avg (mm)",
                            "value": f"{max_val:.3f} / {avg_val:.3f}",
                            "inline": True
                        },
                        {
                            "name": "超標點詳情 (Top 3)",
                            "value": top_3_details if top_3_details != "None" else "無超標點",
                            "inline": False
                        }
                    ],
                    "footer": {
                        "text": f"Machine: {socket.gethostname()}"
                    }
                }
            ]
        }

        try:
            resp = requests.post(DISCORD_WEBHOOK_URL, json=payload, timeout=5)
            if resp.status_code != 204:
                print(f"[Discord] 發送異常: {resp.status_code}")
        except Exception as e:
            print(f"[Discord] 連線錯誤: {e}")

    def process_rebound(self, diff_dphase_path):
        filename = os.path.basename(diff_dphase_path)
        print(f"\n[Process] 處理檔案: {filename}")
        
        if not self.is_scan_2(diff_dphase_path):
            print(f"[Process] Scan 判定略過")
            return False

        dphi_data = self.read_binary_file(diff_dphase_path)
        if dphi_data is None: return False

        valid_indices = [i for i in self.indices if i < len(dphi_data)]
        if not valid_indices: return False
        
        valid_indices_arr = np.array(valid_indices)
        displacements = dphi_data[valid_indices_arr] * self.factor
        abs_displacements = np.abs(displacements) # 取絕對值

        max_rebound = np.max(abs_displacements)
        avg_rebound = np.mean(abs_displacements)
        is_alarm = max_rebound > self.threshold

        # === 1. 紀錄趨勢 CSV (新功能) ===
        # 這裡傳入的是「原始計算出的位移」(dphi * factor)
        # 如果您希望 CSV 紀錄正負值都有，傳入 displacements
        # 如果您希望 CSV 紀錄絕對值(變形量)，傳入 abs_displacements (建議用這個)
        self.save_trend_csv(filename, valid_indices, displacements)

        # === 2. 處理告警邏輯 ===
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
                info = f"ID:{idx} Val:{val:.2f}mm {coord}"
                details_list.append(info)
                print(f"  -> {info}")
            
            top_3_str = "\n".join(details_list)

        print(f"[Result] Max={max_rebound:.3f}mm, Alarm={is_alarm}")
        
        # 3. 寫入警報摘要 CSV
        csv_top_3 = top_3_str.replace("\n", " | ")
        self.save_to_csv(filename, max_rebound, avg_rebound, is_alarm, csv_top_3)
        
        # 4. 發送 Discord
        self.trigger_discord(filename, max_rebound, avg_rebound, is_alarm, top_3_str)
        
        return is_alarm

# =============================
# 主程式
# =============================
def main():
    print(f"[Main] 啟動監測 (間隔 {CHECK_INTERVAL} 秒)...")
    print(f"[Main] 按下 Ctrl + C 停止")
    
    monitor = ArcSARMonitor(CONFIG_PATH, LOG_DIR, DATA_DIR, SCAN_TIME_MIN, SCAN_TIME_MAX)
    last_processed_file = None

    try:
        while True:
            files = glob.glob(os.path.join(DATA_DIR, "*.dphase"))
            if files:
                latest_file = max(files, key=os.path.getmtime)
                
                if latest_file != last_processed_file:                  
                    print(f"\n[Main] 發現新檔: {os.path.basename(latest_file)}")
                    monitor.process_rebound(latest_file)
                    last_processed_file = latest_file
                else:
                    pass
            else:
                print("\r[Main] 無 .dphase 檔案", end="")

            time.sleep(CHECK_INTERVAL)

    except KeyboardInterrupt:
        print("\n[Main] 停止程式。")
        sys.exit(0)
    except Exception as e:
        print(f"\n[Main] 錯誤: {e}")
        sys.exit(1)

if __name__ == "__main__":
    main()