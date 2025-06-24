import math
import customtkinter as ctk
import tkinter as tk

ctk.set_appearance_mode("System")
ctk.set_default_color_theme("blue")

def calculate():
    try:
        # 讀取並處理輸入：將 , 轉為 .
        resistivity = float(entry_resistivity.get().replace(",", "."))
        diameter_mm = float(entry_diameter.get().replace(",", "."))
        length_m = float(entry_length.get().replace(",", "."))
        voltage = float(entry_voltage.get().replace(",", "."))
        load_power = float(entry_load_power.get().replace(",", "."))

        # 計算
        area_mm2 = math.pi * (diameter_mm / 2) ** 2
        resistance_m = resistivity / area_mm2 * 1e6
        wire_resistance = resistance_m * length_m
        load_resistence = voltage ** 2 / load_power
        total_current = voltage / (load_resistence + wire_resistance)
        wire_voltdrop = total_current * wire_resistance
        load_voltdrop = voltage - wire_voltdrop
        lost_persent = (voltage - load_voltdrop) / voltage * 100

        # 顯示輸出
        output_text.delete("1.0", tk.END)
        output_text.insert(tk.END, f"電阻率: {resistivity} Ω·m\n")
        output_text.insert(tk.END, f"導線直徑: {diameter_mm} mm\n")
        output_text.insert(tk.END, f"橫截面面積: {area_mm2:.4f} mm²\n")
        output_text.insert(tk.END, f"每公尺電阻: {resistance_m:.6f} Ω/m\n")
        output_text.insert(tk.END, f"導線長度: {length_m} m\n")
        output_text.insert(tk.END, f"電源電壓: {voltage} v\n")
        output_text.insert(tk.END, f"負載功率: {load_power} w\n")
        output_text.insert(tk.END, f"導線電阻: {wire_resistance * 1e3:.3f} mΩ\n")
        output_text.insert(tk.END, f"導線壓降: {wire_voltdrop:.3f} v\n")
        output_text.insert(tk.END, f"負載電壓: {load_voltdrop:.3f} v\n")
        output_text.insert(tk.END, f"損失占比: {lost_persent:.2f} %\n")

    except ValueError:
        output_text.delete("1.0", tk.END)
        output_text.insert(tk.END, "請輸入有效的數值。")

# 建立視窗
app = ctk.CTk()
app.title("壓降計算器")
app.geometry("270x460")

# ====== 輸入區 ======
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

entry_resistivity = add_labeled_entry(frame_input, "電阻率 (Ω·m)", "1.8065e-8")
entry_diameter = add_labeled_entry(frame_input, "導線直徑 (mm)", "0.5")
entry_length = add_labeled_entry(frame_input, "導線長度 (m)", "1")
entry_voltage = add_labeled_entry(frame_input, "電源電壓 (V)", "5")
entry_load_power = add_labeled_entry(frame_input, "負載功率 (W)", "15")

# ====== 計算按鈕 ======
button = ctk.CTkButton(app, text="計算", command=calculate)
button.pack(pady=10)

# ====== 輸出區 ======
output_text = tk.Text(app, 
                      height=15, 
                      font=("Courier New", 11), 
                      bg="black",       # 背景顏色
                      fg="white",       # 文字顏色
                      insertbackground="white")  # 游標顏色（讓游標在黑底上可見）
output_text.pack(padx=10, pady=10, fill="both", expand=True)

# 綁定 Enter 鍵（Return）
app.bind("<Return>", lambda event: calculate())

# 啟動主程式
app.mainloop()
