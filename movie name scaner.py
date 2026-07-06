import os
import re
import csv
import requests

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
    # 1. 移除副檔名與替換常用分隔符
    name = os.path.splitext(name)[0]
    name = name.replace(".", " ").replace("_", " ")

    # 2. 移除常見的中文盜版網站後綴與無用標籤
    chinese_junk = r"(免費電影線上看|線上看|超清免費在線觀看|高清正片|優酷雲|劇迷|高清追劇首選|中文字幕|简体中字|粵|英語).*"
    name = re.sub(chinese_junk, "", name, flags=re.I)

    year = None
    
    # 3. 尋找年份 (利用 finditer 找最後一個出現的年份，避免誤判片名本身帶有的數字，例如 "1917" 或 "2001太空漫遊")
    matches = list(re.finditer(r"(?:[\s\(\[\-\_])(19\d{2}|20\d{2})(?:[\s\)\]\-\_]|$)", name))
    if matches:
        last_match = matches[-1]
        year = last_match.group(1)
        
        # 關鍵修復：從年份的位置把字串「切斷」，只保留年份之前的字當作片名
        idx = last_match.start()
        if idx > 0: 
            name = name[:idx]
    
    # 4. 移除畫質、壓製組等預設關鍵字 (你原本的 REMOVE_WORDS)
    for word in REMOVE_WORDS:
        name = re.sub(r"\b" + re.escape(word) + r"\b", " ", name, flags=re.I)

    # 5. 移除殘留的括號 (例如未被切乾淨的 [] 或 ())
    name = re.sub(r"\[.*?\]|\(.*?\)", " ", name)
    
    # 6. 移除剩餘的特殊符號 (僅保留英數與中文字)
    name = re.sub(r"[^\w\s\u4e00-\u9fa5]", " ", name)

    # 7. 清理多餘空白
    name = re.sub(r"\s+", " ", name).strip()

    return name, year

#=========================
# 掃描
#=========================

items = []

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

    if js["results"]==[]:
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

results=[]

print()
print("="*60)
print("開始搜尋電影")
print("="*60)

for raw in items:

    title,year=clean_name(raw)

    print()
    print("原始：",raw)
    print("解析：",title)

    movie_id=search_movie(title,year)

    if movie_id is None:

        print("找不到")

        results.append([
            raw,
            title,
            year,
            "",
            ""
        ])

        continue

    info=get_tw_title(movie_id)

    print("英文：",info["original"])
    print("台灣：",info["tw"])

    results.append([
        raw,
        info["original"],
        year,
        info["tw"],
        movie_id
    ])

#=========================
# CSV
#=========================

csv_name="movie_list.csv"

with open(csv_name,"w",newline="",encoding="utf-8-sig") as f:

    writer=csv.writer(f)

    writer.writerow([
        "原始名稱",
        "英文名稱",
        "年份",
        "台灣名稱",
        "TMDB_ID"
    ])

    writer.writerows(results)

print()
print("="*60)
print("完成")
print("輸出:",csv_name)