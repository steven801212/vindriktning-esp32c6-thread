@echo off
setlocal
cd /d "%~dp0"

echo ==========================================================
echo VINDRIKTNING ESP32-C6 Thread Router - CLEAN REBUILD
echo ==========================================================
echo.

if exist .pio (
    echo Removing .pio ...
    rmdir /s /q .pio
)

if exist sdkconfig.seeed_xiao_esp32c6 (
    echo Removing sdkconfig.seeed_xiao_esp32c6 ...
    del /q sdkconfig.seeed_xiao_esp32c6
)

if exist sdkconfig (
    echo Removing sdkconfig ...
    del /q sdkconfig
)

echo.
echo Running PlatformIO clean...
pio run -t clean

echo.
echo Building...
pio run

echo.
if errorlevel 1 (
    echo BUILD FAILED.
    pause
    exit /b 1
)

echo BUILD OK.
pause
