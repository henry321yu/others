@echo off
rem 切換到 UTF-8 編碼
chcp 65001
echo.

(
echo cd /Users/sgrc-325/Desktop
echo "wi-fi 2 usage.py"
) | plink.exe -ssh sgrc-325@10.241.0.114 -pw eos31470.

