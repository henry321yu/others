import os
import georinex as gr
import numpy as np
import matplotlib.pyplot as plt

def skyplot(obs_file, nav_file, output_path):
    try:
        print(f"skyploting...")
        print(f"load obs_file : {obs_file}")
        obs = gr.load(obs_file)
        print(f"load nav_file : {nav_file}")
        nav = gr.load(nav_file)

        # 取得衛星位置資訊
        svs = obs.sv.values
        print(f"取得衛星位置資訊 : {svs}")
        times = obs.time.values
        print(f"取得衛星位置資訊 : {times}")

        fig = plt.figure(figsize=(6, 6))
        ax = fig.add_subplot(111, polar=True)
        ax.set_theta_zero_location('N')  # 北方朝上
        ax.set_theta_direction(-1)       # 順時針方向
        ax.set_rlim(90, 0)                # 天頂為中心，水平線為90度

        plotted_sats = set()

        for sv in svs:
            try:
                el = obs.sel(sv=sv).elevation.values * 180/np.pi  # radians to degrees
                az = obs.sel(sv=sv).azimuth.values * 180/np.pi
                mask = ~np.isnan(el) & ~np.isnan(az)
                print(f"sv : {sv}")
                if np.any(mask):
                    ax.plot(np.deg2rad(az[mask]), el[mask], label=str(sv))
                    plotted_sats.add(sv)
            except Exception as e:
                print(f"跳過 {sv}: {e}")

        ax.set_title(os.path.basename(obs_file))
        ax.legend(loc='lower left', bbox_to_anchor=(1.1, 0.1), fontsize='small', title="Satellites")
        plt.tight_layout()
        plt.savefig(output_path)
        plt.close()
        print(f"[OK] 儲存 skyplot: {output_path}")
    except Exception as e:
        print(f"[錯誤] 處理 {obs_file}: {e}")

def batch_generate_skyplots(folder_path, output_folder):
    os.makedirs(output_folder, exist_ok=True)
    print(f"檔案路徑 : {folder_path}")
    print(f"輸出路徑 : {output_folder}")

    for file in os.listdir(folder_path):
        if file.lower().endswith(".obs"):
            base_name = os.path.splitext(file)[0]
            obs_path = os.path.join(folder_path, file)

            # 嘗試尋找 nav/sbs 檔案
            nav_candidates = [
                base_name + ext for ext in [".nav", ".gnav", ".rnav"]
            ]
            nav_path = None
            print(f"檔案 : {nav_candidates}")
            for candidate in nav_candidates:
                full_path = os.path.join(folder_path, candidate)
                if os.path.exists(full_path):
                    nav_path = full_path
                    break

            if nav_path:
                print(f"得到 : {nav_path}")
                output_path = os.path.join(output_folder, base_name + "_skyplot.jpg")
                skyplot(obs_path, nav_path, output_path)
            else:
                print(f"[略過] 找不到對應 nav 檔: {base_name}")

# ===== 使用方式 =====
input_folder = r"C:\Users\sgrc - 325\Desktop\auto_gps\gps data\output"
output_folder = r"C:\Users\sgrc - 325\Desktop\auto_gps\outputskyplot"

batch_generate_skyplots(input_folder, output_folder)