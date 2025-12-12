@echo off
chcp 65001 >nul

SET CONFIG=C:\Users\b5566\Desktop\hey\git\others\sar\other\yen\config.json
SET DATA_DIR=C:\Users\b5566\Desktop\ftp_download\20250908_TS_test5\cache

echo [INFO] CONFIG = %CONFIG%
echo [INFO] DATA_DIR = %DATA_DIR%
echo.

REM --- 找最新的 .dphase ---
SET LATEST_FILE=

FOR /F "delims=" %%I IN ('dir "%DATA_DIR%\*.dphase" /b /o-d /a-d') DO (
    SET LATEST_FILE=%DATA_DIR%\%%I
    GOTO FOUND
)

:FOUND
echo [INFO] 最新檔案 = %LATEST_FILE%
echo.

REM --- debug print python command ---
echo [DEBUG] 即將執行：
echo python "C:\Users\b5566\Desktop\hey\git\others\sar\other\yen\auto_monitor.py" "%CONFIG%" "%LATEST_FILE%"
echo.

python "C:\Users\b5566\Desktop\hey\git\others\sar\other\yen\auto_monitor.py" "%CONFIG%" "%LATEST_FILE%"
