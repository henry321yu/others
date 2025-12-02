@echo off
chcp 65001 >nul

echo running script.txt ...

%type script.txt | plink -ssh -pw 123 remote@192.168.137.30
type script.txt | plink -ssh -pw 123 remote@192.168.50.146

echo.
echo script finished
pause
