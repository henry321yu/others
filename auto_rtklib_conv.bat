@echo off
setlocal enabledelayedexpansion

set CONVBIN="D:\gps file\12f roof  香菇\demo5_b34k\convbin.exe"
set INPUT_DIR="D:\gps file\auto convert"
set OUTPUT_DIR="D:\gps file\auto convert\output"

REM 建立輸出資料夾（若不存在）
if not exist %OUTPUT_DIR% (
    mkdir %OUTPUT_DIR%
)

cd /d %INPUT_DIR%
for %%f in (*.ubx) do (
    echo 處理中: %%f
    %CONVBIN% -r ubx -o %OUTPUT_DIR%\%%~nf.obs -n %OUTPUT_DIR%\%%~nf.nav -s %OUTPUT_DIR%\%%~nf.sbs %%f
)

echo 所有檔案已處理完畢。
pause
