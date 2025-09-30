import ctypes
dll = ctypes.CDLL(r"C:\Users\sgrc-325\Desktop\git\das\get\DasCardDemodLib.dll")
print(dll.DasCardOpen)