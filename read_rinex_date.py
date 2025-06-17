import os
import csv
from datetime import datetime, timedelta

def parse_obs_header_time(file_path):
    start_time = None
    end_time = None
    with open(file_path, 'r', encoding='utf-8-sig', errors='ignore') as f:
        for line in f:
            if "TIME OF FIRST OBS" in line:
                try:
                    parts = line[:43].split()
                    start_time = datetime(
                        int(parts[0]), int(parts[1]), int(parts[2]),
                        int(parts[3]), int(parts[4]), int(float(parts[5]))
                    )
                except Exception as e:
                    print(f"[錯誤] 解析 start_time 時出錯於 {file_path}: {e}")
            elif "TIME OF LAST OBS" in line:
                try:
                    parts = line[:43].split()
                    end_time = datetime(
                        int(parts[0]), int(parts[1]), int(parts[2]),
                        int(parts[3]), int(parts[4]), int(float(parts[5]))
                    )
                except Exception as e:
                    print(f"[錯誤] 解析 end_time 時出錯於 {file_path}: {e}")

            if start_time and end_time:
                break
    return start_time, end_time

def adjust_utc_to_utc8(dt):
    return dt + timedelta(hours=8) if dt else None

def main(input_folder, output_csv):
    results = []

    for filename in os.listdir(input_folder):
        if filename.lower().endswith('.obs'):
            filepath = os.path.join(input_folder, filename)
            start_time, end_time = parse_obs_header_time(filepath)
            if start_time and end_time:
                start_time_local = adjust_utc_to_utc8(start_time)
                end_time_local = adjust_utc_to_utc8(end_time)
                base_name = os.path.splitext(filename)[0] + '.ubx'
                # time_str = f"{start_time_local.strftime('%Y/%m/%d %H:%M:%S')}, {end_time_local.strftime('%m/%d %H:%M:%S')}"
                start_date = f"{start_time_local.strftime('%Y/%m/%d')}"
                start_time = f"{start_time_local.strftime('%H:%M:%S')}"
                end_date = f"{end_time_local.strftime('%Y/%m/%d')}"
                end_time = f"{end_time_local.strftime('%H:%M:%S')}"
                results.append((base_name, start_date, start_time, end_date, end_time))
                print(f"成功掃描日期並輸出 : {filename}")
            else:
                print(f"無法解析時間區間: {filename}")

    with open(output_csv, 'w', newline='', encoding='utf-8-sig') as csvfile:
        writer = csv.writer(csvfile)
        writer.writerow(['原檔案', '開始日期(UTC+8)', '開始時間(UTC+8)', '結束日期(UTC+8)', '結束時間(UTC+8)'])
        for row in results:
            writer.writerow(row)

    print(f"完成，輸出檔案: {output_csv}")

if __name__ == "__main__":
    input_folder = r"D:\gps file\auto convert\output"  # 修改為你的 rinex 輸出資料夾
    output_csv = r"D:\gps file\auto_rinex_time_ranges.csv"
    main(input_folder, output_csv)
    input("輸出完成，請按任意鍵結束...")
