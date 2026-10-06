# Production Flasher Tool (Công cụ Nạp Firmware Hàng Loạt)

Thư mục này cung cấp công cụ nạp nhanh file `.bin` cho xưởng sản xuất hoặc kỹ thuật viên lắp ráp xe mà không cần cài đặt PlatformIO hay VS Code.

## 1. Yêu cầu:
- Đã cài đặt Python 3 trên máy tính.
- Cài đặt `esptool`:
  ```bash
  pip install esptool
  ```

## 2. Cách nạp nhanh trên Windows:
1. Cắm mạch ESP32 vào cổng USB.
2. Kiểm tra file `firmware.bin` đã được build từ PlatformIO (nằm tại `.pio/build/esp32dev/firmware.bin`).
3. Chạy file `flash_firmware.bat COM_PORT` (Ví dụ: `flash_firmware.bat COM8`).
