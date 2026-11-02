@echo off
echo ========================================================================
echo        Compiling Tempest Federal Bank Core Banking System (C)
echo ========================================================================
gcc -Wall -Wextra -O2 main.c bank_core.c server.c -o bank.exe -lws2_32 -lshell32
if %errorlevel% neq 0 (
    echo [ERROR] Compilation failed!
    pause
    exit /b %errorlevel%
)
echo [SUCCESS] bank.exe built successfully!
echo Run 'bank.exe' for Interactive Console Mode or 'run.bat' for Web Server Mode.
echo.
pause

