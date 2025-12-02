# main_auto_scanner.py
import serial
import time
import logging
from pywinauto.application import Application

# --- 設定區 ---
SERIAL_PORT = 'COM9'     # COM Port
BAUD_RATE = 115200         # 傳輸速率

# --- Log 目錄設定 ---
LOG_DIR = r"D:\auto_logs"
os.makedirs(LOG_DIR, exist_ok=True)   # 若資料夾不存在，自動建立
LOG_FILE = os.path.join(LOG_DIR, "scan_auto.log")  # 完整路徑

# --- Lidar觸發設定 ---
DISTANCE_THRESHOLD = 2         # 門檻距離(m)
TRIGGER_INTERVAL = 20 * 60     # 觸發間隔(秒)
CONSECUTIVE_REQUIRED = 5       # 需要連續 n 次達標

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

        # 2. (可選，但推薦) 讓視窗準備就緒
        main_window.set_focus()
        main_window.wait('ready', timeout=10)

        # 3. 尋找按鈕 (使用 auto_id="1083")
        logging.info(f"正在尋找 auto_id='{SCAN_BUTTON_AUTO_ID}' (運行) 按鈕...")
        scan_button = main_window.child_window(auto_id=SCAN_BUTTON_AUTO_ID, control_type="Button")
        
        # (備用方案：如果 auto_id 失敗，可以改用 title)
        # scan_button = main_window.child_window(title=SCAN_BUTTON_TITLE, control_type="Button")
        
        # 4. 檢查按鈕是否存在並可點擊
        if scan_button.exists() and scan_button.is_enabled():
            logging.info(f"找到按鈕 (auto_id={SCAN_BUTTON_AUTO_ID})，正在執行點擊...")
            
            # .click_input() 是最模擬真人滑鼠點擊的方式
            scan_button.click_input()
            
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

    last_trigger_time = 0
    consecutive_count = 0  # 計數連續達成條件的次數

    while True:
        try:
            with serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=None) as ser:
                #logging.info(f"成功連接到 {SERIAL_PORT}。持續接收距離資料...")
                print(f"成功連接到 {SERIAL_PORT}。持續接收距離資料...")

                while True:
                    line = ser.readline().decode("utf-8").strip()
                    if not line:
                        continue

                    #logging.info(f"收到原始資料: '{line}'")

                    parts = line.split(",")

                    if len(parts) != 3:
                        logging.warning(f"資料格式錯誤: {line}")
                        print("Invalid data:", line)
                        continue

                    try:
                        distance = float(parts[0])
                        strength = int(parts[1])
                        temp = float(parts[2])
                    except ValueError:
                        logging.warning(f"解析錯誤: {line}")
                        print("Parse error:", line)
                        continue

                    # 印出解析成功資料（可移除）
                    # print(f"距離={distance}m, 強度={strength}, 溫度={temp}°C")

                    # 取得現在時間
                    now = time.time()

                    # =========================
                    # ⭐ 觸發條件：距離 <= DISTANCE_THRESHOLD 公尺 並且間隔滿 TRIGGER_INTERVAL 分鐘
                    # =========================
                    if distance <= DISTANCE_THRESHOLD:
                        consecutive_count += 1
                        print(f"[達標] 已連續 {consecutive_count}/{CONSECUTIVE_REQUIRED} 次距離 <= {DISTANCE_THRESHOLD}m")

                        # 若未達到連續要求次數 → 不觸發
                        if consecutive_count < CONSECUTIVE_REQUIRED:
                            continue  

                        # 達標次數達到要求後 → 檢查是否滿足時間間隔
                        if now - last_trigger_time >= TRIGGER_INTERVAL:
                            #logging.info(f"======= 距離連續 {CONSECUTIVE_REQUIRED} 次 <= {DISTANCE_THRESHOLD}m，觸發掃描! =======")
                            print(f"距離連續 {CONSECUTIVE_REQUIRED} 次 <= {DISTANCE_THRESHOLD}m，觸發掃描!")

                            last_trigger_time = now   # 記錄觸發時間
                            consecutive_count = 0      # 重置連續計數

                            success = trigger_scan_pywinauto()

                            if success:
                                logging.info("掃描觸發成功。")
                                print("掃描觸發成功。")
                            else:
                                logging.error("掃描觸發失敗！請檢查日誌。")
                                print("掃描觸發失敗！")

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
                        #if consecutive_count != 0:
                        #    print(f"[重置] 距離 > {DISTANCE_THRESHOLD}m，連續達標次數清空。")
                        consecutive_count = 0

                        # print(f"距離 {distance}m 未達觸發條件。")

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
    
