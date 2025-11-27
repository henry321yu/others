@echo off
chcp 65001 >nul

echo 正在執行 script.txt 內容...

type script.txt | plink -ssh -pw eos31470. sgrc-325@10.241.0.114

echo.
echo 執行完畢
pause
