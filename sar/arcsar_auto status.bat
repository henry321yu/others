@echo off
chcp 65001 >nul

echo Running remote python...

(
    echo cd C:/Users/admin/Desktop
    echo "C:\Users\admin\AppData\Local\Programs\Python\Python311\python.exe" "task status.py"
) | plink -ssh -pw 123 remote@192.168.137.30

echo.
echo Script finished.
pause
