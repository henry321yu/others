import tkinter as tk
from tkinter import filedialog
import pandas as pd
import matplotlib.pyplot as plt
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg, NavigationToolbar2Tk
import glob
import os
import datetime
import re
import numpy as np
import json

class SeismicViewerApp:

    def __init__(self, root):

        self.root = root
        self.root.title("Seismic Viewer")
        self.root.geometry("1200x900")

        self.df = None
        self.cursors = []
        self.annotations = []
        self.cursor_dots = []

        self.config_file = "last_dir.json"
        self.data_dir = os.getcwd()

        self.load_config()

        # ========= UI =========

        self.left = tk.Frame(root,width=260)
        self.left.pack(side=tk.LEFT,fill=tk.Y)

        self.right = tk.Frame(root)
        self.right.pack(side=tk.RIGHT,fill=tk.BOTH,expand=True)

        tk.Button(self.left,text="選擇資料夾",command=self.select_folder).pack(pady=5)

        self.file_listbox = tk.Listbox(self.left,width=35)
        self.file_listbox.pack(side=tk.LEFT,fill=tk.BOTH,expand=True)

        sb = tk.Scrollbar(self.left)
        sb.pack(side=tk.RIGHT,fill=tk.Y)

        self.file_listbox.config(yscrollcommand=sb.set)
        sb.config(command=self.file_listbox.yview)

        self.file_listbox.bind("<<ListboxSelect>>",self.on_file_select)

        # ========= Matplotlib =========

        self.fig, self.axs = plt.subplots(4,1,sharex=True)

        self.canvas = FigureCanvasTkAgg(self.fig,master=self.right)
        self.canvas.get_tk_widget().pack(fill=tk.BOTH,expand=True)

        self.toolbar = NavigationToolbar2Tk(self.canvas,self.right)
        self.toolbar.update()

        self.canvas.mpl_connect("motion_notify_event",self.on_mouse_move)
        self.canvas.mpl_connect("scroll_event",self.on_scroll)

        # ========= single axis buttons =========

        frame = tk.Frame(self.right)
        frame.pack()

        tk.Button(frame,text="X axis",command=lambda:self.open_single_axis('x')).pack(side=tk.LEFT,padx=5)
        tk.Button(frame,text="Y axis",command=lambda:self.open_single_axis('y')).pack(side=tk.LEFT,padx=5)
        tk.Button(frame,text="Z axis",command=lambda:self.open_single_axis('z')).pack(side=tk.LEFT,padx=5)
        tk.Button(frame,text="Total Bias",command=lambda:self.open_single_axis('bias')).pack(side=tk.LEFT,padx=5)

        self.load_files()


    # ================================
    # config
    # ================================

    def load_config(self):

        if not os.path.exists(self.config_file):
            return

        try:
            with open(self.config_file,"r",encoding="utf8") as f:
                cfg=json.load(f)

            d=cfg.get("last_data_dir")

            if d and os.path.isdir(d):
                self.data_dir=d

        except:
            pass

    def save_config(self):

        with open(self.config_file,"w",encoding="utf8") as f:
            json.dump({"last_data_dir":self.data_dir},f,indent=2,ensure_ascii=False)


    # ================================
    # load files
    # ================================

    def load_files(self):

        self.file_listbox.delete(0,tk.END)

        txt_files = glob.glob(os.path.join(self.data_dir,"perm_*.txt"))
        txt_files += glob.glob(os.path.join(self.data_dir,"temp_*.txt"))

        self.files = sorted(txt_files)

        if not self.files:
            self.file_listbox.insert(tk.END,"No files")
            return

        for f in self.files:
            self.file_listbox.insert(tk.END,os.path.basename(f))

        self.file_listbox.selection_set(0)
        self.on_file_select(None)


    def select_folder(self):

        folder=filedialog.askdirectory(initialdir=self.data_dir)

        if not folder:
            return

        self.data_dir=folder
        self.save_config()
        self.load_files()


    # ================================
    # read data
    # ================================

    def read_data(self,filename):

        ext=os.path.splitext(filename)[1].lower()

        # -------- old instrument --------
        if ext==".txt":

            df=pd.read_csv(
                filename,
                header=None,
                names=[
                    'cpu','datetime',
                    'ax','ay','az',
                    'bias','temp',
                    'event','status','freq'
                ]
            )

            df['datetime']=pd.to_datetime(df['datetime'],errors='coerce')

            df['rel_time']=pd.to_numeric(df['cpu'],errors='coerce')

            df['x']=pd.to_numeric(df['ax'],errors='coerce')
            df['y']=pd.to_numeric(df['ay'],errors='coerce')
            df['z']=pd.to_numeric(df['az'],errors='coerce')

            scale=1
            df['x']*=scale
            df['y']*=scale
            df['z']*=scale

            df['bias']=pd.to_numeric(df['bias'],errors='coerce')  # 使用bia資料

        for c in ['rel_time','x','y','z']:
            df[c]=pd.to_numeric(df[c],errors='coerce')

        df.dropna(subset=['rel_time','x','y','z'],inplace=True)
        df.reset_index(drop=True,inplace=True)

        if len(df)==0:
            return None

        df['v']=np.sqrt(df['x']**2+df['y']**2+df['z']**2)

        return df


    # ================================
    # file select
    # ================================

    def on_file_select(self,event):

        sel=self.file_listbox.curselection()

        if not sel:
            return

        filename=self.files[sel[0]]

        df=self.read_data(filename)

        if df is None:
            return

        self.df=df

        self.plot_data(os.path.basename(filename))


    # ================================
    # plot
    # ================================

    def plot_data(self,name):

        df=self.df

        self.fig.clf()

        self.axs=self.fig.subplots(
            4,1,
            sharex=True,
            gridspec_kw={'height_ratios':[1.7,1,1,1]}
        )

        self.cursors=[]
        self.annotations=[]
        self.cursor_dots=[]

        cols=['v','x','y','z']
        colors=['blue','red','green','purple']
        labels=['Vector','X','Y','Z']

        for i,ax in enumerate(self.axs):

            ax.plot(df['rel_time'],df[cols[i]],color=colors[i],lw=1)

            ax.set_ylabel(labels[i])
            ax.grid(True)

            line=ax.axvline(0,color='black',linestyle=':')
            line.set_visible(False)

            txt=ax.text(
                0.01,0.7,"",
                transform=ax.transAxes,
                bbox=dict(boxstyle="round",fc="white",alpha=0.5)
            )
            txt.set_visible(False)

            dot,=ax.plot([],[],'o',color='black')
            dot.set_visible(False)

            self.cursors.append(line)
            self.annotations.append(txt)
            self.cursor_dots.append(dot)

        peak = df['v'].loc[df['v'].abs().idxmax()]
        self.axs[0].set_title(f"{name}   Peak: {peak:.3f} g")

        self.axs[-1].set_xlabel("Time (sec)")

        self.fig.tight_layout()
        xmin = df['rel_time'].min()
        xmax = df['rel_time'].max()

        for ax in self.axs:
            ax.set_xlim(xmin, xmax)

        self.canvas.draw()


    # ================================
    # cursor
    # ================================

    def on_mouse_move(self,event):

        if self.df is None or event.xdata is None:

            for l in self.cursors:
                l.set_visible(False)

            for t in self.annotations:
                t.set_visible(False)

            for d in self.cursor_dots:
                d.set_visible(False)

            self.canvas.draw_idle()
            return

        x=event.xdata

        idx=(np.abs(self.df['rel_time']-x)).argmin()

        t=self.df['rel_time'].iloc[idx]
        dt=self.df['datetime'].iloc[idx]

        time_str = dt.strftime("%H:%M:%S.%f")[:-3] if pd.notna(dt) else ""

        cols=['v','x','y','z']

        for i,ax in enumerate(self.axs):

            val=self.df[cols[i]].iloc[idx]

            self.cursors[i].set_xdata([t])
            self.cursors[i].set_visible(True)

            self.cursor_dots[i].set_data([t],[val])
            self.cursor_dots[i].set_visible(True)

            self.annotations[i].set_text(
                f"{t:.3f}s {time_str}\n{val:.3f} g"
            )

            self.annotations[i].set_visible(True)

        self.canvas.draw_idle()


    # ================================
    # zoom
    # ================================

    def on_scroll(self,event):

        if not self.cursor_dots:
            return

        x=self.cursor_dots[0].get_xdata()

        if len(x)==0:
            return

        center=x[0]

        scale=1.1

        factor=1/scale if event.step>0 else scale

        for ax in self.axs:

            xmin,xmax=ax.get_xlim()

            new_width=(xmax-xmin)*factor

            new_xmin=center-(center-xmin)*factor

            ax.set_xlim(new_xmin,new_xmin+new_width)

        self.canvas.draw_idle()


    # ================================
    # single axis
    # ================================

    def open_single_axis(self,axis):

        if self.df is None:
            return

        win=tk.Toplevel(self.root)

        win.title(axis)
        win.geometry("900x600")

        fig,ax=plt.subplots()

        canvas=FigureCanvasTkAgg(fig,master=win)
        canvas.get_tk_widget().pack(fill=tk.BOTH,expand=True)

        NavigationToolbar2Tk(canvas,win)

        t=self.df['rel_time']
        v=self.df[axis]

        ax.plot(t,v,lw=1)

        peak = v.loc[v.abs().idxmax()]
        ax.set_title(f"axis: {axis}   Peak: {peak:.3f} g")
        ax.set_xlabel("Time")
        ax.set_ylabel("g")
        ax.grid(True)
        
        xmin = t.min()
        xmax = t.max()
        ax.set_xlim(xmin, xmax)

        # ===== 加入游標 + 縮放 =====

        line=ax.axvline(0,color='black',linestyle=':')
        line.set_visible(False)

        dot,=ax.plot([],[],'o',color='black')
        dot.set_visible(False)

        txt=ax.text(0.02,0.9,"",
                    transform=ax.transAxes,
                    bbox=dict(boxstyle="round",fc="white",alpha=0.5))
        txt.set_visible(False)

        def on_move(event):

            if event.xdata is None:
                line.set_visible(False)
                dot.set_visible(False)
                txt.set_visible(False)
                canvas.draw_idle()
                return

            x=event.xdata
            idx=(np.abs(self.df['rel_time']-x)).argmin()

            t=self.df['rel_time'].iloc[idx]
            val=self.df[axis].iloc[idx]
            dt=self.df['datetime'].iloc[idx]

            time_str = dt.strftime("%H:%M:%S.%f")[:-3] if pd.notna(dt) else ""

            line.set_xdata([t])
            line.set_visible(True)

            dot.set_data([t],[val])
            dot.set_visible(True)

            txt.set_text(f"{t:.3f}s {time_str}\n{val:.3f} g")
            txt.set_visible(True)

            canvas.draw_idle()

        def on_scroll(event):

            if event.xdata is None:
                return

            center=event.xdata
            scale=1.3

            factor=1/scale if event.step>0 else scale

            xmin,xmax=ax.get_xlim()

            new_width=(xmax-xmin)*factor
            new_xmin=center-(center-xmin)*factor

            ax.set_xlim(new_xmin,new_xmin+new_width)

            canvas.draw_idle()

        canvas.mpl_connect("motion_notify_event",on_move)
        canvas.mpl_connect("scroll_event",on_scroll)

        canvas.draw()


# ================================

if __name__=="__main__":

    root=tk.Tk()

    app=SeismicViewerApp(root)

    root.mainloop()