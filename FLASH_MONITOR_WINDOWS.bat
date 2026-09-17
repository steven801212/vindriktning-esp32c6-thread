@echo off
setlocal
cd /d "%~dp0"

echo ==========================================================
echo VINDRIKTNING ESP32-C6 Thread Router - UPLOAD + MONITOR
echo ==========================================================
echo.
echo Close any other serial monitor before continuing.
echo.

pio run -t upload
if errorlevel 1 (
    echo.
    echo UPLOAD FAILED.
    echo If needed, enter bootloader mode and retry.
    pause
    exit /b 1
)

echo.
echo Starting serial monitor at 115200...
echo Exit PlatformIO monitor with Ctrl+C.
echo.
pio device monitor -b 115200
