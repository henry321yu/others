import tkinter as tk
from tkinter import ttk
import pandas as pd
import matplotlib.pyplot as plt
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg, NavigationToolbar2Tk
import glob
import os
import datetime
import re
import numpy as np
from tkinter import filedialog
import json


class SeismicViewerApp:
    def __init__(self, root):
        self.root = root
        self.root.title("地震儀資料繪圖工具 (Seismic CSV Viewer)")
        self.root.geometry("1200x900") # 稍微加大高度以容納 Toolbar

        # --- 變數初始化 ---
        self.df = None
        self.cursors = [] # 儲存垂直線
        self.annotations = [] # 儲存數值文字
        self.cursor_dots = []  # 游標對應的資料點黑點

        # ===== 設定檔 =====
        self.config_file = "last_dir.json"        
        self.data_dir = os.getcwd() # 預設資料夾        
        self.load_config() # 嘗試讀取上次資料夾

        # --- 介面佈局 ---
        self.left_frame = tk.Frame(root, width=250, bg='white')
        self.left_frame.pack(side=tk.LEFT, fill=tk.Y, padx=5, pady=5)
        
        self.right_frame = tk.Frame(root, bg='white')
        self.right_frame.pack(side=tk.RIGHT, fill=tk.BOTH, expand=True, padx=5, pady=5)

        # 檔案列表區
        self.lbl_list = tk.Label(self.left_frame, text="檔案列表", bg='white', font=('Arial', 10))
        self.lbl_list.pack(pady=2)

        self.btn_select_dir = tk.Button(
            self.left_frame,
            text="選擇資料夾",
            command=self.select_folder
        )
        self.btn_select_dir.pack(pady=5)
        
        self.file_listbox = tk.Listbox(self.left_frame, selectmode=tk.SINGLE, font=('Arial', 10), width=35)
        self.file_listbox.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)
        
        self.scrollbar = tk.Scrollbar(self.left_frame)
        self.scrollbar.pack(side=tk.RIGHT, fill=tk.Y)
        self.file_listbox.config(yscrollcommand=self.scrollbar.set)
        self.scrollbar.config(command=self.file_listbox.yview)

        self.file_listbox.bind('<<ListboxSelect>>', self.on_file_select)

        # 繪圖區
        self.fig, self.axs = plt.subplots(3, 1, figsize=(8, 6), sharex=True)
        self.plt_canvas = FigureCanvasTkAgg(self.fig, master=self.right_frame)
        self.plt_canvas.get_tk_widget().pack(fill=tk.BOTH, expand=True)
        self.plt_canvas.mpl_connect('scroll_event', self.on_scroll)

        # 加入 Matplotlib 工具列 (縮放/存檔等功能)
        self.toolbar = NavigationToolbar2Tk(self.plt_canvas, self.right_frame)
        self.toolbar.update()
        self.plt_canvas.get_tk_widget().pack(fill=tk.BOTH, expand=True) # 重新 Pack 確保順序

        # ===== 單軸放大按鈕區 =====
        self.single_axis_frame = tk.Frame(self.right_frame)
        self.single_axis_frame.pack(fill=tk.X, pady=5)

        tk.Button(self.single_axis_frame, text="Z axis", width=12,
                  command=lambda: self.open_single_axis_window('z')).pack(side=tk.LEFT, padx=5)

        tk.Button(self.single_axis_frame, text="X axis", width=12,
                  command=lambda: self.open_single_axis_window('x')).pack(side=tk.LEFT, padx=5)

        tk.Button(self.single_axis_frame, text="Y axis", width=12,
                  command=lambda: self.open_single_axis_window('y')).pack(side=tk.LEFT, padx=5)

        # 綁定滑鼠移動事件 (用於顯示數值)
        self.plt_canvas.mpl_connect('motion_notify_event', self.on_mouse_move)

        # 載入檔案
        self.csv_files = []
        self.load_files()

    def load_config(self):
        if not os.path.exists(self.config_file):
            return

        try:
            with open(self.config_file, "r", encoding="utf-8") as f:
                cfg = json.load(f)
                last_dir = cfg.get("last_data_dir")

                if last_dir and os.path.isdir(last_dir):
                    self.data_dir = last_dir
        except Exception as e:
            print(f"Config load failed: {e}")

    def save_config(self):
        try:
            with open(self.config_file, "w", encoding="utf-8") as f:
                json.dump(
                    {"last_data_dir": self.data_dir},
                    f,
                    indent=2,
                    ensure_ascii=False
                )
        except Exception as e:
            print(f"Config save failed: {e}")

    def load_files(self):
        self.file_listbox.delete(0, tk.END)

        search_path = os.path.join(self.data_dir, "*.csv")
        self.csv_files = sorted(glob.glob(search_path))

        if not self.csv_files:
            self.file_listbox.insert(tk.END, "無 CSV 檔案")
            return

        for f in self.csv_files:
            # Listbox 只顯示檔名，不顯示完整路徑
            self.file_listbox.insert(tk.END, os.path.basename(f))

        self.file_listbox.selection_set(0)
        self.on_file_select(None)

    def select_folder(self):
        folder = filedialog.askdirectory(
            title="選擇 CSV 資料夾",
            initialdir=self.data_dir
        )
        if not folder:
            return

        self.data_dir = folder
        self.save_config() #儲存設定檔
        self.load_files()

    def parse_filename_datetime(self, filename):
        match = re.search(r'(\d{8})(\d{6})', filename)
        if match:
            date_part = match.group(1)
            time_part = match.group(2)
            formatted_time = f"{time_part[:2]}:{time_part[2:4]}:{time_part[4:]}"
            return f"{date_part} {formatted_time}"
        return filename

    def on_file_select(self, event):
        selection = self.file_listbox.curselection()
        if not selection:
            return
        filename = self.file_listbox.get(selection[0])
        if filename == "無 CSV 檔案":
            return

        fullpath = os.path.join(self.data_dir, filename)
        self.plot_data(fullpath)

    def plot_data(self, filename):
        try:
            # 讀取資料
            df = pd.read_csv(filename, comment='#', sep=r'[,\s]+', engine='python',
                             header=None, names=['epoch', 'z', 'x', 'y', 'r0', 'r1', 'r2'],
                             on_bad_lines='skip')
            
            for col in df.columns:
                df[col] = pd.to_numeric(df[col], errors='coerce')
            df.dropna(inplace=True)
            df.reset_index(drop=True, inplace=True)

            if df.empty:
                print(f"檔案 {filename} 無有效數據")
                return

            t0 = df['epoch'].iloc[0]
            df['rel_time'] = df['epoch'] - t0

            # === 計算合向量 Magnitude ===
            df['v'] = np.sqrt(df['z']**2 + df['x']**2 + df['y']**2)

            self.df = df  # 存入 self，供滑鼠事件使用

            # 調整 figure 為 4 行子圖 (合向量 + Z,X,Y)
            self.fig.clf()
            self.axs = self.fig.subplots(4, 1, sharex=True,
                                         gridspec_kw={'height_ratios': [1.7, 1, 1, 1]})

            # 清除舊圖與舊的 Cursor 物件
            self.cursors = []
            self.annotations = []
            self.cursor_dots = []

            # 繪圖函式 (包含靜態 Peak 標籤)
            def plot_and_tag(ax, col_name, label_text, color):
                ax.plot(df['rel_time'], df[col_name], color=color, lw=1, label=label_text)
                ax.set_ylabel("gal")
                ax.grid(True, linestyle='--', alpha=0.5)
                ax.legend(loc='upper right', fontsize='small')

                # 標記最大值 (靜態)
                if len(df) > 0:
                    idx_max = df[col_name].abs().idxmax()
                    val_max = df[col_name].iloc[idx_max]
                    time_rel = df['rel_time'].iloc[idx_max]
                    time_str = datetime.datetime.fromtimestamp(df['epoch'].iloc[idx_max]).strftime("%H:%M:%S.%f")[:-3]
                    
                    y_min, y_max = ax.get_ylim()
                    rng = y_max - y_min if y_max != y_min else 1.0
                    offset = 0.25 if val_max >= 0 else -0.45
                    
                    ax.annotate(f"{val_max:.2f}\n{time_str}", 
                                xy=(time_rel, val_max), 
                                xytext=(time_rel, val_max + rng * offset),
                                arrowprops=dict(facecolor='black', arrowstyle='->'),
                                fontsize=8, ha='center',
                                bbox=dict(boxstyle="round,pad=0.2", fc="white", alpha=0.7))

                # 初始化動態 Cursor (垂直線與文字)
                # 垂直線 (初始隱藏)
                line = ax.axvline(x=0, color='black', linestyle=':', linewidth=1, alpha=0.8)
                line.set_visible(False)
                self.cursors.append(line)

                # 左上角數值文字 (初始隱藏)
                # transform=ax.transAxes 代表使用相對座標 (0,0 為左下, 1,1 為右上)
                text = ax.text(0.02, 0.9, "", transform=ax.transAxes, 
                               bbox=dict(boxstyle="round", fc="white", alpha=0.3),
                               verticalalignment='top', fontsize=9, fontweight='bold')
                text.set_visible(False)
                self.annotations.append(text)

                # 游標資料點黑點（初始隱藏）
                dot, = ax.plot([], [], 'o', color='black', markersize=5, zorder=10)
                dot.set_visible(False)
                self.cursor_dots.append(dot)

            # 依序繪圖：合向量 + Z,X,Y
            plot_and_tag(self.axs[0], 'v', 'Vector Mag', 'blue')
            plot_and_tag(self.axs[1], 'z', 'Z', 'red')
            plot_and_tag(self.axs[2], 'x', 'X', 'green')
            plot_and_tag(self.axs[3], 'y', 'Y', 'purple')

            file_datetime_str = self.parse_filename_datetime(filename)
            pga = df['v'].abs().max()
            self.axs[0].set_title(f"{file_datetime_str}  PGA: {pga:.1f}gal", fontsize=14, fontweight='bold')
            self.axs[3].set_xlabel("Time (sec)")

            self.fig.tight_layout()
            self.plt_canvas.draw()

        except Exception as e:
            print(f"Error: {e}")

    def open_single_axis_window(self, axis_name):
        if self.df is None:
            return

        axis_map = {
            'z': ('Z', 'red'),
            'x': ('X', 'green'),
            'y': ('Y', 'purple')
        }

        label, color = axis_map[axis_name]

        # ===== 新視窗 =====
        win = tk.Toplevel(self.root)
        win.title(f"{label} 放大檢視")
        win.geometry("900x600")

        fig, ax = plt.subplots(figsize=(9, 5))
        canvas = FigureCanvasTkAgg(fig, master=win)
        canvas.get_tk_widget().pack(fill=tk.BOTH, expand=True)

        toolbar = NavigationToolbar2Tk(canvas, win)
        toolbar.update()

        # ===== 資料 =====
        t = self.df['rel_time']
        v = self.df[axis_name]

        ax.plot(t, v, color=color, lw=1)
        ax.set_xlabel("Time (sec)")
        ax.set_ylabel("gal")
        ax.set_title(label)
        ax.grid(True, linestyle='--', alpha=0.5)

        # ===== Peak 標示 =====
        idx = v.abs().idxmax()
        peak_t = t.iloc[idx]
        peak_v = v.iloc[idx]
        peak_time_str = datetime.datetime.fromtimestamp(
            self.df['epoch'].iloc[idx]
        ).strftime("%H:%M:%S.%f")[:-3]

        # ==================================================
        # 跳窗專用 Cursor（垂直線 + 數值文字）
        # ==================================================

        cursor_line = ax.axvline(
            x=0, color='black', linestyle=':', linewidth=1, alpha=0.8
        )
        cursor_line.set_visible(False)

        # 黑點 (初始隱藏)
        cursor_dot, = ax.plot([], [], 'o', color='black', markersize=5, zorder=10)
        cursor_dot.set_visible(False)

        cursor_text = ax.text(
            0.02, 0.95, "",
            transform=ax.transAxes,
            bbox=dict(boxstyle="round", fc="white", alpha=0.3),
            verticalalignment='top',
            fontsize=9,
            fontweight='bold'
        )
        cursor_text.set_visible(False)

        # ===== 滑鼠移動事件（只作用在此跳窗）=====
        def on_move(event):
            if event.inaxes != ax or event.xdata is None:
                cursor_line.set_visible(False)
                cursor_text.set_visible(False)
                cursor_dot.set_visible(False)
                canvas.draw_idle()
                return

            x_mouse = event.xdata
            idx = (np.abs(self.df['rel_time'] - x_mouse)).argmin()

            t_cur = self.df['rel_time'].iloc[idx]
            v_cur = self.df[axis_name].iloc[idx]
            epoch = self.df['epoch'].iloc[idx]

            time_str = datetime.datetime.fromtimestamp(epoch).strftime("%H:%M:%S.%f")[:-3]

            cursor_line.set_xdata([t_cur])
            cursor_line.set_visible(True)

            cursor_text.set_text(
                f"Time: {t_cur:.2f}s ({time_str})\nVal: {v_cur:.3f} gal"
            )
            cursor_text.set_visible(True)

            # 更新黑點
            cursor_dot.set_data([t_cur], [v_cur])
            cursor_dot.set_visible(True)

            canvas.draw_idle()

        canvas.mpl_connect('motion_notify_event', on_move)

        fig.tight_layout()
        canvas.draw()
        def on_scroll(event):
            """以跳窗黑點為中心縮放 X 軸"""
            if cursor_dot is None:
                return

            xdata = cursor_dot.get_xdata()
            if len(xdata) == 0:
                return
            x_center = xdata[0]

            base_scale = 1.2
            scale_factor = 1 / base_scale if event.step > 0 else base_scale

            x_min, x_max = ax.get_xlim()
            new_width = (x_max - x_min) * scale_factor
            new_xmin = x_center - (x_center - x_min) * scale_factor
            new_xmax = new_xmin + new_width
            ax.set_xlim(new_xmin, new_xmax)

            canvas.draw_idle()

        canvas.mpl_connect('scroll_event', on_scroll)

    def on_mouse_move(self, event):
        """處理滑鼠移動，顯示十字線與數值"""
        if self.df is None or event.inaxes is None:
            # 滑鼠不在圖表內，隱藏所有 Cursor
            for line in self.cursors: line.set_visible(False)
            for txt in self.annotations: txt.set_visible(False)
            for dot in self.cursor_dots: dot.set_visible(False)
            self.plt_canvas.draw_idle()
            return

        # 取得滑鼠所在的 X 座標 (時間)
        x_mouse = event.xdata
        
        # 找出最接近該時間的資料索引
        # 使用 numpy searchsorted 或 abs().argmin()
        # 這裡假設時間是排序的，用 searchsorted 會更快，但 argmin 更直觀
        idx = (np.abs(self.df['rel_time'] - x_mouse)).argmin()
        
        current_time = self.df['rel_time'].iloc[idx]
        current_epoch = self.df['epoch'].iloc[idx]
        
        # 轉換絕對時間字串
        dt_str = datetime.datetime.fromtimestamp(current_epoch).strftime("%H:%M:%S.%f")[:-3]

        # 更新三個圖表的顯示
        cols = ['v', 'z', 'x', 'y']
        for i, ax in enumerate(self.axs):
            val = self.df[cols[i]].iloc[idx]
            
            # 更新垂直線位置
            self.cursors[i].set_xdata([current_time])
            self.cursors[i].set_visible(True)

            # 更新黑點（資料點）
            self.cursor_dots[i].set_data([current_time], [val])
            self.cursor_dots[i].set_visible(True)
            
            # 更新文字
            txt_content = f"Time: {current_time:.2f}s ({dt_str})\nVal: {val:.3f} gal"
            self.annotations[i].set_text(txt_content)
            self.annotations[i].set_visible(True)

        # 使用 draw_idle 優化效能 (只在空閒時重繪)
        self.plt_canvas.draw_idle()

    def on_scroll(self, event):
        """以游標黑點位置為中心，縮放 X 軸"""
        if self.df is None or not self.cursor_dots:
            return

        # 取三軸黑點中任一軸的 X 座標（游標位置）
        x_center = self.cursor_dots[0].get_xdata()
        if len(x_center) == 0:
            return  # 黑點還沒出現
        x_center = x_center[0]

        # 計算縮放比例
        base_scale = 1.1  # 每次滾輪縮放 10%
        if event.step > 0:
            scale_factor = 1 / base_scale  # 放大
        else:
            scale_factor = base_scale      # 縮小

        for ax in self.axs:
            xlim = ax.get_xlim()
            x_min, x_max = xlim

            # 計算新範圍
            new_width = (x_max - x_min) * scale_factor
            new_xmin = x_center - (x_center - x_min) * scale_factor
            new_xmax = new_xmin + new_width

            ax.set_xlim(new_xmin, new_xmax)

        self.plt_canvas.draw_idle()

if __name__ == "__main__":
    root = tk.Tk()
    app = SeismicViewerApp(root)
    root.mainloop()