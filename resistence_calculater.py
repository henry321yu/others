import math

# 銅的電阻率 (ohm⋅meter)
# resistivity = 1.68e-8 #wiki copper
resistivity = 1.8065e-8

# 銅線直徑（mm）與長度（m）
diameter_mm = 1.3
length_m = 1

# 換算成米並計算橫截面面積 A = πr^2
area_mm2 = math.pi * (diameter_mm / 2) ** 2

# 每公尺的電阻
resistance_per_meter = resistivity / area_mm2 * 1e6

# 計算總電阻
wire_resistance = resistance_per_meter * length_m

# voltage = 110
voltage = 5
load_power = 10 #walt
load_resistence = voltage ** 2 / load_power

total_current = voltage / ( load_resistence + wire_resistance )

wire_voltdrop = total_current * wire_resistance
load_voltdrop = voltage - wire_voltdrop



# 輸出
print(f"銅的電阻率: {resistivity} ohm·m")
print(f"導線直徑: {diameter_mm} mm")
print(f"橫截面面積: {area_mm2:.4f} mm²")
print(f"每公尺電阻: {resistance_per_meter:.6f} ohm/m")
print(f"導線長度: {length_m} m")
print(f"電源電壓: {voltage} v")
print(f"負載功率: {load_power} w")
print(f"導線電阻: {wire_resistance*1e3:.3f} mohm")
print(f"導線壓降: {wire_voltdrop:.3f} v")
print(f"負載電壓: {load_voltdrop:.3f} v")