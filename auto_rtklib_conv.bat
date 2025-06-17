@echo off
setlocal enabledelayedexpansion

set CONVBIN="C:\Users\sgrc - 325\Desktop\auto_gps\demo5_b34k\convbin.exe"
set INPUT_DIR="C:\Users\sgrc - 325\Desktop\auto_gps\gps data"
set OUTPUT_DIR="C:\Users\sgrc - 325\Desktop\auto_gps\gps data\output"

REM �إ߿�X��Ƨ��]�Y���s�b�^
if not exist %OUTPUT_DIR% (
    mkdir %OUTPUT_DIR%
)

cd /d %INPUT_DIR%
for %%f in (*.ubx) do (
    echo �B�z��: %%f
    %CONVBIN% -r ubx -o %OUTPUT_DIR%\%%~nf.obs -n %OUTPUT_DIR%\%%~nf.nav -s %OUTPUT_DIR%\%%~nf.sbs %%f
)

echo �Ҧ��ɮפw�B�z�����C
pause
