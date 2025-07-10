import os

def get_folder_size(folder, show_progress=False):
    total_size = 0
    for dirpath, dirnames, filenames in os.walk(folder):
        for f in filenames:
            try:
                fp = os.path.join(dirpath, f)
                if os.path.isfile(fp):
                    total_size += os.path.getsize(fp)
                    if show_progress:
                        print(f"\r目前大小：{format_size(total_size)}", end='', flush=True)
            except Exception as e:
                print(f"\n⚠️ 無法讀取檔案 {fp}: {e}")
    if show_progress:
        print()  # 換行
    return total_size

def format_size(size_bytes):
    if size_bytes >= 1024 ** 3:
        return f"{size_bytes / (1024 ** 3):.2f} GB"
    elif size_bytes >= 1024 ** 2:
        return f"{size_bytes / (1024 ** 2):.2f} MB"
    elif size_bytes >= 1024:
        return f"{size_bytes / 1024:.2f} KB"
    else:
        return f"{size_bytes} Bytes"

def list_folder_sizes(path='.'):
    folder_sizes = []
    total_size_all = 0

    all_entries = [entry for entry in os.listdir(path) if os.path.isdir(os.path.join(path, entry))]
    total = len(all_entries)

    print("🔍 開始掃描資料夾...\n")

    for idx, entry in enumerate(all_entries, start=1):
        full_path = os.path.join(path, entry)
        print(f"[{idx}/{total}] 掃描中：{entry}")
        size_bytes = get_folder_size(full_path, show_progress=True)
        total_size_all += size_bytes
        folder_sizes.append((entry, size_bytes))
        print(f"{entry:<30}\t{format_size(size_bytes)}\n")

    print("\n📊 📁 資料夾硬碟使用狀況（由大到小）")
    print("────────────────────────────────────────────")
    folder_sizes.sort(key=lambda x: x[1], reverse=True)

    for folder, size in folder_sizes:
        print(f"{folder:<30}\t{format_size(size)}")

    print("\n🧮 總共使用空間：", format_size(total_size_all))

if __name__ == "__main__":
    list_folder_sizes()
    input("\n✅ 掃描完畢，按 Enter 結束 ...")
