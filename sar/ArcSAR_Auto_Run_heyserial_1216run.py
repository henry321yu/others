# main_auto_scanner.py
import serial
import time
import logging
from pywinauto.application import Application
import os

# --- 設定區 ---
SERIAL_PORT = 'COM9'     # COM Port
BAUD_RATE = 115200         # 傳輸速率

# --- Log 目錄設定 ---
LOG_DIR = r"D:\auto_logs"
os.makedirs(LOG_DIR, exist_ok=True)   # 若資料夾不存在，自動建立

# 自動建立 log 檔名：scan_auto_yymmdd_hhmmss.log
timestamp = time.strftime("%y%m%d_%H%M%S")  # 例：2025-12-04 → 251204_153210
LOG_FILE = os.path.join(LOG_DIR, f"scan_auto_{timestamp}.log")

# --- Lidar觸發設定 ---
DISTANCE_THRESHOLD = 4         # 門檻距離(m)
TRIGGER_INTERVAL = 6 * 60     # 觸發間隔(秒) - 兩次"有車事件"之間的冷卻時間
CONSECUTIVE_REQUIRED = 5       # 需要連續 n 次達標

# ==========================================
# [新增] 二次掃描 (無車背景掃描) 設定
# ==========================================
SECONDARY_SCAN_DELAY = 2 * 60  # 第一次掃描後，等待多少秒執行第二次掃描
                               # 例如：5 * 60 = 300 秒 (5分鐘)
                               # 設定為 0 則不執行此功能
# ==========================================

# --- pywinauto 設定 (根據您的偵測結果) ---
ARCSAR_WINDOW_TITLE = ".*全景邊坡監測雷達 系統控制與監測預警軟體.*" # ArcSAR 視窗標題
# 將優先使用 auto_id，因為它最可靠
SCAN_BUTTON_AUTO_ID = "1083"      # '運行' 按鈕的 auto_id
SCAN_BUTTON_TITLE = "運行"        # (備用) 這是 title
# -----------------------------------------------------

# --- 設定日誌 ---
logging.basicConfig(filename=LOG_FILE, 
                    level=logging.INFO, 
                    format='%(asctime)s - %(levelname)s - %(message)s')

def trigger_scan_pywinauto():
    """
    使用 pywinauto 連接到 ArcSAR 並點擊 '運行' (auto_id="1083") 按鈕。
    """
    try:
        logging.info(f"pywinauto 啟動：正在連接到 '{ARCSAR_WINDOW_TITLE}'...")
        
        # 1. 連接到已經在執行的程式
        app = Application(backend="uia").connect(title_re=ARCSAR_WINDOW_TITLE)
        main_window = app.window(title_re=ARCSAR_WINDOW_TITLE)
        logging.info("成功連接到主視窗。")

        # 3. 尋找按鈕 (使用 auto_id="1083")
        logging.info(f"正在尋找 auto_id='{SCAN_BUTTON_AUTO_ID}' (運行) 按鈕...")
        scan_button = main_window.child_window(auto_id=SCAN_BUTTON_AUTO_ID, control_type="Button")
        
        # 4. 檢查按鈕是否存在並可點擊
        if scan_button.exists() and scan_button.is_enabled():
            logging.info(f"找到按鈕 (auto_id={SCAN_BUTTON_AUTO_ID})，正在執行點擊...")
            
            # .click_input() 是最模擬真人滑鼠點擊的方式
            scan_button.invoke()
            
            logging.info("pywinauto 點擊「運行」按鈕... 成功。")
            return True
        else:
            logging.warning(f"警告：找到了視窗，但找不到 auto_id='{SCAN_BUTTON_AUTO_ID}' 按鈕或按鈕無法點擊。")
            return False

    except Exception as e:
        logging.error(f"pywinauto 執行失敗：{e}")
        print(f"pywinauto 執行失敗：{e} (詳見 {LOG_FILE})")
        return False

def main_listener():
    """主監聽迴圈：等待來自 COM 的 distance,strength,temp 資料並在距離 <= DISTANCE_THRESHOLD 時觸發"""

    logging.info(f"腳本啟動。開始監聽 {SERIAL_PORT} (速率: {BAUD_RATE})...")
    print(f"腳本啟動。開始監聽 {SERIAL_PORT}...")
    
    if SECONDARY_SCAN_DELAY > 0:
        print(f"功能提示：已啟用二次掃描，將在觸發後等待 {SECONDARY_SCAN_DELAY} 秒再次掃描。")
    else:
        print("功能提示：二次掃描功能已關閉 (Delay = 0)。")

    last_trigger_time = 0
    consecutive_count = 0  # 計數連續達成條件的次數

    while True:
        try:
            with serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=None) as ser:
                print(f"成功連接到 {SERIAL_PORT}。持續接收距離資料...")

                while True:
                    line = ser.readline().decode("utf-8").strip()
                    if not line:
                        continue

                    parts = line.split(",")

                    if len(parts) != 3:
                        logging.warning(f"資料格式錯誤: {line}")
                        continue

                    try:
                        distance = float(parts[0])
                        # strength = int(parts[1])
                        # temp = float(parts[2])
                    except ValueError:
                        logging.warning(f"解析錯誤: {line}")
                        continue

                    # 取得現在時間
                    now = time.time()

                    # =========================
                    # ⭐ 觸發條件：距離 <= DISTANCE_THRESHOLD 公尺
                    # =========================
                    if distance >= 4:
                        consecutive_count += 1
                        print(f"[達標] 已連續 {consecutive_count}/{CONSECUTIVE_REQUIRED} 次距離 >= {DISTANCE_THRESHOLD}m")

                        # 若未達到連續要求次數 → 不觸發
                        if consecutive_count < CONSECUTIVE_REQUIRED:
                            continue  

                        # 達標次數達到要求後 → 檢查是否滿足時間間隔
                        if now - last_trigger_time >= TRIGGER_INTERVAL:
                            print(f"距離連續 {CONSECUTIVE_REQUIRED} 次 <= {DISTANCE_THRESHOLD}m，觸發第一次掃描 (有車)!")
                            logging.info(f"觸發條件達成：距離 {distance}m。開始第一次掃描流程。")

                            last_trigger_time = now   # 記錄觸發時間 (鎖定下一次有車觸發)
                            consecutive_count = 0     # 重置連續計數
                            
                            # 在呼叫 GUI 點擊前加入等待 12/16
                            DELAY_BEFORE_RUN = 2.1
                            logging.info(f"等待 {DELAY_BEFORE_RUN} 秒後再執行「運行」點擊...")
                            print(f"等待 {DELAY_BEFORE_RUN} 秒後再執行「運行」...")
                            # 選擇性：清除舊的 Serial 緩衝，避免等待期間累積的舊資料影響後續判斷
                            ser.reset_input_buffer()
                            time.sleep(DELAY_BEFORE_RUN)

                            # --- 執行第一次掃描 ---
                            success = trigger_scan_pywinauto()

                            if success:
                                logging.info("第一次掃描觸發成功。")
                                print("第一次掃描觸發成功。")

                                # =========================================
                                # [新增邏輯] 檢查並執行二次掃描 (無車背景)
                                # =========================================
                                if SECONDARY_SCAN_DELAY > 0:
                                    wait_msg = f"等待 {SECONDARY_SCAN_DELAY} 秒後執行二次掃描 (背景)..."
                                    logging.info(wait_msg)
                                    print(f"--- {wait_msg} ---")
                                    
                                    # 程式暫停等待 (注意：此時不會讀取 Serial，數據會堆積在緩衝區)
                                    time.sleep(SECONDARY_SCAN_DELAY)
                                    
                                    logging.info("等待結束，開始執行二次掃描...")
                                    print("等待結束，觸發二次掃描...")
                                    
                                    success_2 = trigger_scan_pywinauto()
                                    if success_2:
                                        logging.info("二次掃描觸發成功。")
                                        print("二次掃描觸發成功。")
                                    else:
                                        logging.error("二次掃描觸發失敗！")
                                        print("二次掃描觸發失敗！")

                                    # [關鍵] 清除 Serial 輸入緩衝區
                                    # 因為等待期間 Serial 資料一直進來，如果不清除，
                                    # 下一次迴圈會讀到 5 分鐘前的舊資料，可能導致錯誤判斷。
                                    logging.info("清除 Serial 緩衝區舊資料...")
                                    ser.reset_input_buffer()
                                # =========================================

                            else:
                                logging.error("第一次掃描觸發失敗！跳過二次掃描。")
                                print("第一次掃描觸發失敗！")

                            logging.info("======= 掃描流程結束。返回監聽狀態 =======")
                            print("======= 掃描流程結束。返回監聽狀態 =======\n")
                        else:
                            # 間隔時間不足
                            remaining = int(TRIGGER_INTERVAL - (now - last_trigger_time))
                            mins = remaining // 60
                            secs = remaining % 60
                            print(f"距離達觸發條件，但仍需等待 {mins} 分 {secs} 秒 才能再次觸發。")

                    else:
                        # 未達標 → 清空連續計數器
                        consecutive_count = 0

        except serial.SerialException as e:
            logging.error(f"序列埠 {SERIAL_PORT} 錯誤: {e}。將在 5 秒後重試...")
            print(f"序列埠 {SERIAL_PORT} 錯誤: {e}。將在 5 秒後重試...")
            time.sleep(5)
        except Exception as e:
            logging.error(f"發生未預期的錯誤: {e}。將在 5 秒後重試...")
            print(f"發生未預期的錯誤: {e}。將在 5 秒後重試...")
            time.sleep(5)

# --- 啟動主程式 ---
if __name__ == "__main__":
    main_listener()