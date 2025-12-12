@echo off
chcp 65001 >nul

echo Running remote commands...

(
    echo schtasks /query /tn arcsar_auto
    echo schtasks /run /tn arcsar_auto
    echo exit
) | plink -ssh -pw 123 remote@192.168.137.30

echo.
echo Script finished.
pause
