# test_button_click.py
# (這是一個 *僅供測試* 的腳本，用來驗證 pywinauto 的點擊功能)

import time
import logging
from pywinauto.application import Application

# --- 設定區 ---
LOG_FILE = 'scan_auto_test.log' # 使用一個不同的日誌檔

# --- pywinauto 設定 (與主腳本相同) ---
ARCSAR_WINDOW_TITLE = ".*全景邊坡監測雷達 系統控制與監測預警軟體.*" # 您的 ArcSAR 視窗標題
SCAN_BUTTON_AUTO_ID = "1083"      # '運行' 按鈕的 auto_id
# -----------------------------------------------------

# --- 設定日誌 ---
logging.basicConfig(filename=LOG_FILE, 
                    level=logging.INFO, 
                    format='%(asctime)s - %(levelname)s - %(message)s')

def trigger_scan_pywinauto():
    """
    (這部分與主腳本完全相同)
    使用 pywinauto 連接到 ArcSAR 並點擊 '運行' (auto_id="1083") 按鈕。
    """
    try:
        logging.info(f"pywinauto 啟動：正在連接到 '{ARCSAR_WINDOW_TITLE}'...")
        
        app = Application(backend="uia").connect(title_re=ARCSAR_WINDOW_TITLE)
        main_window = app.window(title_re=ARCSAR_WINDOW_TITLE)
        logging.info("成功連接到主視窗。")

        main_window.set_focus()
        main_window.wait('ready', timeout=10)

        logging.info(f"正在尋找 auto_id='{SCAN_BUTTON_AUTO_ID}' (運行) 按鈕...")
        scan_button = main_window.child_window(auto_id=SCAN_BUTTON_AUTO_ID, control_type="Button")
        
        if scan_button.exists() and scan_button.is_enabled():
            logging.info(f"找到按鈕 (auto_id={SCAN_BUTTON_AUTO_ID})，正在執行點擊...")
            scan_button.click_input()
            logging.info("pywinauto 點擊「運行」按鈕... 成功。")
            print(">>> 點擊「運行」按鈕... 成功。")
            return True
        else:
            logging.warning(f"警告：找到了視窗，但找不到 auto_id='{SCAN_BUTTON_AUTO_ID}' 按鈕或按鈕無法點擊。")
            print(f"!!! 警告：找不到 auto_id='{SCAN_BUTTON_AUTO_ID}' 按鈕。")
            return False

    except Exception as e:
        logging.error(f"pywinauto 執行失敗：{e}")
        print(f"!!! pywinauto 執行失敗：{e} (詳見 {LOG_FILE})")
        return False

def test_manual_trigger():
    """
    手動觸發迴圈：
    用 input() 取代 serial.readline()
    """
    logging.info("測試腳本啟動。")
    print("=== 歡迎使用 ArcSAR 自動點擊測試程式 ===")
    print(f"    - 將點擊 ArcSAR 視窗 (標題: {ARCSAR_WINDOW_TITLE})")
    print(f"    - 上的按鈕 (auto_id: {SCAN_BUTTON_AUTO_ID})")
    print("\n請確認 ArcSAR 軟體已經開啟並顯示在畫面上。")
    print("---------------------------------------------")

    while True:
        try:
            # 這是關鍵：程式會停在這裡，直到您按下 Enter
            input("\n>>> 請按下 [Enter] 鍵來模擬訊號，觸發一次「運行」點擊 (或按 Ctrl+C 結束)...\n")
            
            # --- 模擬訊號觸發 ---
            logging.info(f"======= 收到 [Enter] 手動觸發! 開始執行點擊! =======")
            print("...收到模擬訊號！開始執行點擊...")
            
            success = trigger_scan_pywinauto()
            
            if success:
                logging.info("手動觸發測試成功。")
            else:
                logging.error("手動觸發測試失敗！請檢查日誌。")
            
            logging.info(f"======= 點擊流程結束。等待下一次 [Enter] 觸發 =======")

        except KeyboardInterrupt:
            print("\n測試結束。再見！")
            logging.info("使用者按下 Ctrl+C，測試腳本結束。")
            break
        except Exception as e:
            logging.error(f"發生未預期的錯誤: {e}")
            print(f"發生錯誤: {e}")
            time.sleep(2)

# --- 啟動主程式 ---
if __name__ == "__main__":
    # (重要) 再次提醒: 
    # 如果 ArcSAR 是用「系統管理員」權限執行的，
    # 這個 .py 腳本也必須「以系統管理員身分執行」。
    test_manual_trigger()