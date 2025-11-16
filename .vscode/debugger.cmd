@echo off
setlocal

REM Ten skrypt siedzi w .vscode, więc przechodzimy do katalogu projektu
cd /d "%~dp0\.."

echo Flashing application...
st-flash write myproject.bin 0x08010000
if errorlevel 1 (
    echo st-flash failed
    pause
    exit /b 1
)

echo Starting st-util...
start "st-util" st-util

REM małe opóźnienie, żeby st-util się podniósł
timeout /t 1 >nul

echo Starting GDB...
arm-none-eabi-gdb.exe %*

endlocal
