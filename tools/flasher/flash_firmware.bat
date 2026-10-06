@echo off
set PORT=%1
if "%PORT%"=="" set PORT=COM8

echo ========================================================
echo   ESP32 Production Firmware Flasher
echo   Target Port: %PORT%
echo ========================================================

esptool.py --chip esp32 --port %PORT% --baud 921600 --before default_reset --after hard_reset write_flash -z --flash_mode dio --flash_freq 40m --flash_size detect 0x10000 ..\..\.pio\build\esp32dev\firmware.bin

if %ERRORLEVEL% EQU 0 (
    echo [SUCCESS] Nap Firmware hoan tat thanh cong!
) else (
    echo [ERROR] Nap Firmware that bai. Kiem tra lai cong COM hoac nhan giu nut BOOT!
)
pause
