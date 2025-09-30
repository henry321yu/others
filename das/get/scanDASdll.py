import os
import ctypes

dlls = [f for f in os.listdir('.') if f.lower().endswith('.dll')]

for d in dlls:
    try:
        lib = ctypes.CDLL(d)
        func = getattr(lib, "DasCardOpen", None)
        if func:
            print(f"✅ Found target DLL: {d}")
            break;
        else:
            print(f"❌ {d} does not export DasCardOpen")
    except Exception as e:
        print(f"⚠️ Error loading {d}: {e}")
