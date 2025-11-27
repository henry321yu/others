@echo off
chcp 65001 >nul

echo running script.txt ...

type script.txt | plink -batch -ssh -pw eos31470. sgrc-325@10.241.0.114

echo.
echo script finished
pause
