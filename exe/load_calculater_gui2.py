import math
import customtkinter as ctk
import tkinter as tk
import configparser
import os

CONFIG_FILE = "config.ini"

# 建立預設設定檔
def create_default_config():
    config = configparser.ConfigParser(interpolation=None)
    config["Inputs"] = {
        "resistivity": "1.8065e-8",
        "diameter": "0.5",
        "cores": "1",
        "length": "1",
        "voltage": "5",
        "load_power": "15",
        "k": "8.8"
    }
    config["OutputFormat"] = {
        "resistivity": "{:.4e} Ω·m",
        "diameter_mm": "{:.2f} mm",
        "cores": "{:.0f}",
        "area_mm2": "{:.4f} mm²",
        "resistance_m": "{:.6f} Ω/m",
        "length_m": "{:.2f} m",
        "voltage": "{:.2f} V",
        "load_power": "{:.2f} W",
        "wire_resistance": "{:.3f} mΩ",
        "wire_voltdrop": "{:.3f} V",
        "load_voltdrop": "{:.3f} V",
        "lost_percent": "{:.2f} %",
        "k": "{:.2f}",
        "lim_current": "{:.3f} A"
    }
    with open(CONFIG_FILE, "w", encoding="utf-8") as configfile:
        config.write(configfile)

# 讀取設定檔
def load_config():
    if not os.path.exists(CONFIG_FILE):
        create_default_config()
    config = configparser.ConfigParser(interpolation=None)
    config.read(CONFIG_FILE, encoding="utf-8")
    return config

# 儲存目前輸入值到 ini 檔
def save_config():
    config["Inputs"]["resistivity"] = entry_resistivity.get()
    config["Inputs"]["diameter"] = entry_diameter.get()
    config["Inputs"]["cores"] = entry_cores.get()
    config["Inputs"]["length"] = entry_length.get()
    config["Inputs"]["voltage"] = entry_voltage.get()
    config["Inputs"]["load_power"] = entry_load_power.get()
    config["Inputs"]["k"] = entry_k.get()
    with open(CONFIG_FILE, "w", encoding="utf-8") as configfile:
        config.write(configfile)

# 設定主題
ctk.set_appearance_mode("System")
ctk.set_default_color_theme("blue")

# 初始化設定
config = load_config()

# 建立主視窗
app = ctk.CTk()
app.title("Calculator")
app.geometry("270x570")

# ===== 輸入區 =====
frame_input = ctk.CTkFrame(app)
frame_input.pack(pady=10)

def add_labeled_entry(parent, label_text, default_value):
    frame = ctk.CTkFrame(parent)
    frame.pack(pady=2, fill="x", padx=10)

    label = ctk.CTkLabel(frame, text=label_text, width=120, anchor="w")
    label.pack(side="left", padx=(0, 10))

    entry = ctk.CTkEntry(frame)
    entry.insert(0, default_value)
    entry.pack(side="left", fill="x", expand=True)
    return entry

entry_resistivity = add_labeled_entry(frame_input, "電阻率 (Ω·m)", config["Inputs"].get("resistivity", "1.8065e-8"))
entry_diameter = add_labeled_entry(frame_input, "導線直徑 (mm)", config["Inputs"].get("diameter", "0.5"))
entry_cores = add_labeled_entry(frame_input, "股數 c", config["Inputs"].get("cores", "1"))
entry_length = add_labeled_entry(frame_input, "導線長度 (m)", config["Inputs"].get("length", "1"))
entry_voltage = add_labeled_entry(frame_input, "電源電壓 (V)", config["Inputs"].get("voltage", "5"))
entry_load_power = add_labeled_entry(frame_input, "負載功率 (W)", config["Inputs"].get("load_power", "15"))
entry_k = add_labeled_entry(frame_input, "安全係數 k", config["Inputs"].get("k", "8.8"))

# ===== 計算函式 =====
def calculate():
    try:
        resistivity = float(entry_resistivity.get().replace(",", "."))
        diameter_mm = float(entry_diameter.get().replace(",", "."))
        cores = float(entry_cores.get().replace(",", "."))
        length_m = float(entry_length.get().replace(",", "."))
        voltage = float(entry_voltage.get().replace(",", "."))
        load_power = float(entry_load_power.get().replace(",", "."))
        k = float(entry_k.get().replace(",", "."))

        area_mm2 = math.pi * (diameter_mm / 2) ** 2
        if cores > 1: area_mm2 = (math.pi * (diameter_mm) ** 2) / 4 * cores
        resistance_m = resistivity / area_mm2 * 1e6
        wire_resistance = resistance_m * length_m
        load_resistence = voltage ** 2 / load_power
        total_current = voltage / (load_resistence + wire_resistance)
        wire_voltdrop = total_current * wire_resistance
        load_voltdrop = voltage - wire_voltdrop
        lost_percent = (voltage - load_voltdrop) / voltage * 100
        lim_current = k * area_mm2

        # 輸出格式設定
        f_resistivity = config["OutputFormat"].get("resistivity", "{:.5f} Ω·m").format(resistivity)
        f_diameter_mm = config["OutputFormat"].get("diameter_mm", "{:.3f} mm").format(diameter_mm)
        f_cores = config["OutputFormat"].get("cores", "{:.0f}").format(cores)
        f_area_mm2 = config["OutputFormat"].get("area_mm2", "{:.3f} mm²").format(area_mm2)
        f_resistance_m = config["OutputFormat"].get("resistance_m", "{:.3f} Ω/m").format(resistance_m)
        f_length_m = config["OutputFormat"].get("length_m", "{:.3f} m").format(length_m)
        f_voltage = config["OutputFormat"].get("voltage", "{:.3f} V").format(voltage)
        f_load_power = config["OutputFormat"].get("load_power", "{:.3f} W").format(load_power)
        f_wire_resistance = config["OutputFormat"].get("wire_resistance", "{:.3f} mΩ").format(wire_resistance * 1e3)
        f_wire_voltdrop = config["OutputFormat"].get("wire_voltdrop", "{:.3f} V").format(wire_voltdrop)
        f_load_voltdrop = config["OutputFormat"].get("load_voltdrop", "{:.3f} V").format(load_voltdrop)
        f_lost_percent = config["OutputFormat"].get("lost_percent", "{:.2f} %").format(lost_percent)
        f_k = config["OutputFormat"].get("k", "{:.2f} %").format(k)
        f_lim_current = config["OutputFormat"].get("lim_current", "{:.2f} %").format(lim_current)

        # 顯示輸出
        output_text.delete("1.0", tk.END)
        output_text.insert(tk.END, f"電阻率: {f_resistivity}\n")
        output_text.insert(tk.END, f"導線直徑: {f_diameter_mm}\n")
        output_text.insert(tk.END, f"股數: {f_cores}\n")
        output_text.insert(tk.END, f"橫截面面積: {f_area_mm2}\n")
        output_text.insert(tk.END, f"每公尺電阻: {f_resistance_m}\n")
        output_text.insert(tk.END, f"導線長度: {f_length_m}\n")
        output_text.insert(tk.END, f"電源電壓: {f_voltage}\n")
        output_text.insert(tk.END, f"負載功率: {f_load_power} \n")
        output_text.insert(tk.END, f"導線電阻: {f_wire_resistance}\n")
        output_text.insert(tk.END, f"導線壓降: {f_wire_voltdrop}\n")
        output_text.insert(tk.END, f"負載電壓: {f_load_voltdrop}\n")
        output_text.insert(tk.END, f"損失占比: {f_lost_percent}\n")
        output_text.insert(tk.END, f"安全係數: {f_k}\n")
        output_text.insert(tk.END, f"安全電流: {f_lim_current}\n")

        # 儲存設定
        save_config()

    except ValueError:
        output_text.delete("1.0", tk.END)
        output_text.insert(tk.END, "請輸入有效的數值。")

# ===== 計算按鈕 =====
button = ctk.CTkButton(app, text="計算", command=calculate)
button.pack(pady=10)

# ===== 輸出區 =====
output_text = tk.Text(app, height=15, font=("Consolas", 11), bg="black", fg="white", insertbackground="white")
output_text.pack(padx=10, pady=10, fill="both", expand=True)

# 綁定 Enter 鍵
app.bind("<Return>", lambda event: calculate())

# 啟動主程式
app.mainloop()