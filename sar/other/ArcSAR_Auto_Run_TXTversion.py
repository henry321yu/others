# main_auto_scanner.py
import serial
import time
import logging
from pywinauto.application import Application

# --- 設定區 ---
SERIAL_PORT = 'COM3'     # COM Port
BAUD_RATE = 9600         # 傳輸速率
TRIGGER_SIGNAL = "TRIG"  # (重要!) 觸發訊號, 請再次確認內容
LOG_FILE = 'scan_auto.log'

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
    """主監聽迴圈：等待來自 COM3 的訊號"""
    logging.info(f"腳本啟動。開始監聽 {SERIAL_PORT} (速率: {BAUD_RATE})...")
    print(f"腳本啟動。開始監聽 {SERIAL_PORT}...")
    
    while True: 
        try:
            with serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=None) as ser:
                logging.info(f"成功連接到 {SERIAL_PORT}。等待訊號 '{TRIGGER_SIGNAL}'...")
                print(f"成功連接到 {SERIAL_PORT}。等待訊號...")
                
                while True: 
                    signal = ser.readline().decode('utf-8').strip()
                    if not signal:
                        continue
                        
                    logging.info(f"收到原始訊號: '{signal}'")
                    
                    if signal == TRIGGER_SIGNAL:
                        logging.info(f"======= 收到觸發訊號! 開始執行掃描! =======")
                        print(f"收到觸發訊號! 開始執行 '運行'!")
                        
                        success = trigger_scan_pywinauto()
                        
                        if success:
                            logging.info("掃描觸發成功。")
                            print("掃描觸發成功。")
                        else:
                            logging.error("掃描觸發失敗！請檢查日誌。")
                            print("掃描觸發失敗！")
                        
                        logging.info(f"======= 掃描流程結束。返回監聽狀態 =======")
                        print("======= 掃描流程結束。返回監聽狀態 =======")
                        
                    else:
                        logging.warning(f"收到非預期的訊號: '{signal}' (預期為: '{TRIGGER_SIGNAL}')")
                        print(f"收到非預期訊號: '{signal}'")

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
	