import math
from datetime import datetime, timedelta, timezone

def solar_position(lat, lon, dt_utc):
    """計算指定UTC時間與地點的太陽仰角與方位角"""

    # 1. 轉換時間為儒略日
    timestamp = dt_utc.timestamp()
    jd = timestamp / 86400.0 + 2440587.5
    n = jd - 2451545.0

    # 2. 太陽平均近點角 (degree)
    g = (357.529 + 0.98560028 * n) % 360
    g_rad = math.radians(g)

    # 3. 太陽平黃經 (degree)
    q = (280.459 + 0.98564736 * n) % 360

    # 4. 太陽黃經
    L = (q + 1.915 * math.sin(g_rad) + 0.020 * math.sin(2*g_rad)) % 360
    L_rad = math.radians(L)

    # 5. 地球傾角
    e = 23.439 - 0.00000036 * n
    e_rad = math.radians(e)

    # 6. 太陽赤經與赤緯
    RA = math.degrees(math.atan2(math.cos(e_rad) * math.sin(L_rad), math.cos(L_rad)))
    RA = RA % 360
    dec = math.degrees(math.asin(math.sin(e_rad) * math.sin(L_rad)))

    # 7. 格林威治平時角 (degree)
    GMST = (280.1600 + 360.9856235 * n) % 360

    # 8. 地方時角 (degree)
    LST = (GMST + lon) % 360
    HA = (LST - RA) % 360
    if HA > 180:
        HA -= 360
    HA_rad = math.radians(HA)

    # 9. 轉換為地平座標
    lat_rad = math.radians(lat)
    dec_rad = math.radians(dec)

    alt = math.degrees(math.asin(math.sin(lat_rad)*math.sin(dec_rad) + 
                                 math.cos(lat_rad)*math.cos(dec_rad)*math.cos(HA_rad)))

    az = math.degrees(math.atan2(-math.sin(HA_rad),
                                 math.tan(dec_rad)*math.cos(lat_rad) - math.sin(lat_rad)*math.cos(HA_rad)))
    az = (az + 360) % 360

    return alt, az, dec, RA, HA

if __name__ == "__main__":
    # 互動輸入
    lat = 22.997814
    lon = 120.221267
    local_time_str = "2025-08-06 17:53"

    # 解析台灣時間 (UTC+8)
    local_dt = datetime.strptime(local_time_str, "%Y-%m-%d %H:%M")
    tz_tw = timezone(timedelta(hours=8))
    local_dt = local_dt.replace(tzinfo=tz_tw)

    # 轉換成 UTC
    dt_utc = local_dt.astimezone(timezone.utc)

    # 計算太陽位置
    alt, az, dec, RA, HA = solar_position(lat, lon, dt_utc)

    # 詳細輸出
    print("\n=== 太陽位置計算結果 ===")
    print(f"輸入地點：緯度 {lat}°，經度 {lon}°")
    print(f"台灣時間：{local_dt.strftime('%Y-%m-%d %H:%M:%S %Z')}")
    print(f"UTC時間： {dt_utc.strftime('%Y-%m-%d %H:%M:%S %Z')}")
    print(f"太陽仰角(高度角)：{alt:.2f}°")
    print(f"太陽方位角：{az:.2f}° (北=0°，東=90°)")
    print(f"太陽赤緯：{dec:.2f}°")
    print(f"太陽赤經：{RA:.2f}°")
    print(f"地方時角：{HA:.2f}°")
