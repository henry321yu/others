import psutil
import time
from collections import deque

# 設定要監控的 ZeroTier 網卡名稱
ZEROTIER_INTERFACE = '乙太網路'  # 可換成自動尋找程式或實際介面名稱

# 儲存最近 k 秒的 TX 和 RX 資料（Byte）
k = 2
tx_history = deque(maxlen=k)
rx_history = deque(maxlen=k)

def get_interface_bytes(interface):
    counters = psutil.net_io_counters(pernic=True)
    if interface in counters:
        stats = counters[interface]
        return stats.bytes_sent, stats.bytes_recv
    else:
        raise Exception(f"找不到網卡介面：{interface}")

def format_speed(bytes_per_sec):
    """根據大小轉換單位，顯示為 bit/s 或 Byte/s 到 GB/s"""
    bit_per_sec = bytes_per_sec * 8
    units = [
        ('Gbps', 1_000_000_000),
        ('Mbps', 1_000_000),
        ('Kbps', 1_000),
        ('bps', 1),
    ]
    for unit, factor in units:
        if bit_per_sec >= factor:
            return f"{bit_per_sec / factor:.2f} {unit}"

    return f"{bit_per_sec:.2f} bps"

def format_bytes(bytes_per_sec):
    """根據大小轉換 Byte 單位"""
    units = [
        ('GB/s', 1024 ** 3),
        ('MB/s', 1024 ** 2),
        ('KB/s', 1024),
        ('B/s', 1),
    ]
    for unit, factor in units:
        if bytes_per_sec >= factor:
            return f"{bytes_per_sec / factor:.2f} {unit}"
    return f"{bytes_per_sec:.2f} B/s"

def main():
    print(f"正在監控介面：{ZEROTIER_INTERFACE}")
    prev_tx, prev_rx = get_interface_bytes(ZEROTIER_INTERFACE)

    while True:
        time.sleep(1)
        curr_tx, curr_rx = get_interface_bytes(ZEROTIER_INTERFACE)
        delta_tx = curr_tx - prev_tx
        delta_rx = curr_rx - prev_rx

        tx_history.append(delta_tx)
        rx_history.append(delta_rx)

        avg_tx = sum(tx_history) / len(tx_history)
        avg_rx = sum(rx_history) / len(rx_history)
        avg_total = avg_tx + avg_rx

        print(f"[{time.strftime('%H:%M:%S')}] TX: {format_bytes(avg_tx)} ({format_speed(avg_tx)}), "
              f"RX: {format_bytes(avg_rx)} ({format_speed(avg_rx)}), "
              f"Total: {format_bytes(avg_total)} ({format_speed(avg_total)})")

        prev_tx, prev_rx = curr_tx, curr_rx

if __name__ == "__main__":
    main()
