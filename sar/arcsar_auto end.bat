@echo off
chcp 65001 >nul

echo Running remote commands...

(
    echo schtasks /query /tn arcsar_auto
    echo schtasks /end /tn arcsar_auto
    echo exit
) | plink -ssh -pw 123 remote@192.168.50.146

echo.
echo Script finished.
pause
