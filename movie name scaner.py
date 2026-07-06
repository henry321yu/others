import os
import re
import requests
import openpyxl
from openpyxl.styles import Font, PatternFill, Alignment, Border, Side
from openpyxl.utils import get_column_letter

#=========================
# 設定
#=========================

ROOT = r"G:\old_HDD\film"
API_KEY = "af246bcc8feca07801bb5c59cd22694a"

VIDEO_EXT = {
    ".mkv",".mp4",".avi",".mov",".wmv",".iso",
    ".m2ts",".ts",".mpg",".mpeg",".flv"
}

#=========================
# 這裡是你自定義 Excel 欄寬的地方
# 可以隨時調整下方的數字 (代表字元寬度)
#=========================
EXCEL_COLUMN_WIDTHS = {
    "原始名稱": 55,
    "英文名稱": 35,
    "年份": 10,
    "台灣名稱": 25,
    # "TMDB_ID": 15
}

#=========================
# 清理片名
#=========================

REMOVE_WORDS = [
    "1080p","720p","2160p","480p",
    "BluRay","Blu-Ray","WEBRip","WEB-DL",
    "HDRip","BRRip","DVDRip","Remux",
    "H264","H265","x264","x265",
    "HEVC","AVC",
    "AAC","AC3","DTS","DDP","Atmos",
    "TRUEHD","HDMA",
    "10bit","8bit",
    "HDR","DV",
    "ITA","ENG","JPN","CHS","CHT",
    "MULTI","SUB","SUBS",
]

def clean_name(name):
    name = os.path.splitext(name)[0]
    name = name.replace(".", " ").replace("_", " ")

    chinese_junk = r"(免費電影線上看|線上看|超清免費在線觀看|高清正片|優酷雲|劇迷|高清追劇首選|中文字幕|简体中字|粵|英語).*"
    name = re.sub(chinese_junk, "", name, flags=re.I)

    year = None
    matches = list(re.finditer(r"(?:[\s\(\[\-\_])(19\d{2}|20\d{2})(?:[\s\)\]\-\_]|$)", name))
    if matches:
        last_match = matches[-1]
        year = last_match.group(1)
        idx = last_match.start()
        if idx > 0: 
            name = name[:idx]
    
    for word in REMOVE_WORDS:
        name = re.sub(r"\b" + re.escape(word) + r"\b", " ", name, flags=re.I)

    name = re.sub(r"\[.*?\]|\(.*?\)", " ", name)
    name = re.sub(r"[^\w\s\u4e00-\u9fa5]", " ", name)
    name = re.sub(r"\s+", " ", name).strip()

    return name, year

#=========================
# 掃描
#=========================

items = []

# 為了防止執行出錯，如果找不到資料夾先跳過
if os.path.exists(ROOT):
    for obj in os.scandir(ROOT):
        if obj.is_file():
            ext = os.path.splitext(obj.name)[1].lower()
            if ext in VIDEO_EXT:
                items.append(obj.name)
        elif obj.is_dir():
            items.append(obj.name)

print("="*60)
print("掃描到：")
print("="*60)

for x in items:
    print(x)
print()

#=========================
# TMDB
#=========================

def search_movie(title, year):
    url = "https://api.themoviedb.org/3/search/movie"
    params = {
        "api_key":API_KEY,
        "query":title,
        "language":"en-US"
    }
    if year:
        params["year"]=year

    r = requests.get(url,params=params)
    if r.status_code!=200:
        return None

    js = r.json()
    if js.get("results")==[]:
        return None

    return js["results"][0]["id"]

def get_tw_title(movie_id):
    url=f"https://api.themoviedb.org/3/movie/{movie_id}"
    params={
        "api_key":API_KEY,
        "language":"zh-TW"
    }

    r=requests.get(url,params=params)
    if r.status_code!=200:
        return None

    js=r.json()
    return {
        "tw":js.get("title",""),
        "original":js.get("original_title",""),
        "release":js.get("release_date","")
    }

#=========================
# 搜尋
#=========================

results = []

print()
print("="*60)
print("開始搜尋電影")
print("="*60)

for raw in items:
    title, year = clean_name(raw)

    print()
    print("原始：", raw)
    print("解析：", title)

    movie_id = search_movie(title, year)

    if movie_id is None:
        print("找不到")
        results.append([raw, title, year, "", ""])
        continue

    info = get_tw_title(movie_id)
    print("英文：", info["original"])
    print("台灣：", info["tw"])

    results.append([
        raw,
        info["original"],
        year,
        info["tw"],
        movie_id
    ])

#=========================
# 匯出 Excel (XLSX)
#=========================

excel_name = "movie_list.xlsx"

# 建立活頁簿
wb = openpyxl.Workbook()
ws = wb.active
ws.title = "電影清單"

headers = ["原始名稱", "英文名稱", "年份", "台灣名稱", "TMDB_ID"]
ws.append(headers)

# 寫入資料
for row in results:
    ws.append(row)

# 定義美化樣式
header_fill = PatternFill(start_color="1F4E78", end_color="1F4E78", fill_type="solid") # 深藍色標頭
header_font = Font(name="微軟正黑體", size=11, bold=True, color="FFFFFF")
alt_fill = PatternFill(start_color="F2F6F9", end_color="F2F6F9", fill_type="solid")    # 交替橫條紋淺色
data_font = Font(name="微軟正黑體", size=10)
thin_border = Side(style='thin', color='D9D9D9')
cell_border = Border(left=thin_border, right=thin_border, top=thin_border, bottom=thin_border)

# 1. 處理標題列 與 設定欄寬
for col_idx, header in enumerate(headers, 1):
    cell = ws.cell(row=1, column=col_idx)
    cell.fill = header_fill
    cell.font = header_font
    cell.alignment = Alignment(horizontal="center", vertical="center")
    
    # 根據最上方定義的字典設定寬度
    col_letter = get_column_letter(col_idx)
    ws.column_dimensions[col_letter].width = EXCEL_COLUMN_WIDTHS.get(header, 20)

# 2. 處理資料列 (框線、字型、置中、斑馬紋色)
for row_idx in range(2, ws.max_row + 1):
    is_even = (row_idx % 2 == 0)
    for col_idx in range(1, len(headers) + 1):
        cell = ws.cell(row=row_idx, column=col_idx)
        cell.font = data_font
        cell.border = cell_border
        
        # 偶數列填上淺灰色交替背景
        if is_even:
            cell.fill = alt_fill
            
        # 讓年份與ID置中，名稱靠左
        header_name = headers[col_idx - 1]
        if header_name in ["年份", "TMDB_ID"]:
            cell.alignment = Alignment(horizontal="center", vertical="center")
        else:
            cell.alignment = Alignment(horizontal="left", vertical="center")

# 3. 凍結首列與啟用篩選功能
ws.freeze_panes = "A2"
ws.auto_filter.ref = f"A1:{get_column_letter(len(headers))}{ws.max_row}"

# 儲存
wb.save(excel_name)

print()
print("="*60)
print("完成")
print("輸出:", excel_name)