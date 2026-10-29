# Nhận CAN từ Kan_sim / ACC

Firmware LED mặc định khởi tạo TWAI ở NORMAL, 500 kbit/s, TX GPIO21 và RX GPIO22.
Không dùng GPIO4/5 trên LED: hai chân đó đã dành cho HUB75. NORMAL cung cấp ACK
cho ESP32 phát; firmware LED không gửi application frame.

## Đấu nối hai ESP32

| SN65HVD230 phía LED | ESP32 LED |
|---|---|
| VCC | 3V3 (không dùng 5V) |
| GND | GND |
| CTX / TXD | GPIO21 |
| CRX / RXD | GPIO22 |

Phía phát Kan_sim vẫn dùng TX GPIO5 → CTX, RX GPIO4 ← CRX. Nối H ↔ H, L ↔ L,
GND chung giữa hai module. Hai đầu bus cần mỗi đầu một điện trở 120 Ω giữa H/L;
không thêm nếu module đã có sẵn. Tắt toàn bộ nguồn trước khi đo; cả bus có đủ
hai điện trở thường đo khoảng 60 Ω. Dùng dây H/L xoắn đôi, transceiver active.

## Nạp và chạy

```powershell
cd D:\SEP\IoT-Device
pio run -e esp32dev
pio run -e esp32dev -t upload --upload-port COM8
pio device monitor --port COM8 --baud 115200
```

COM8 chỉ là ví dụ. Chọn đúng board LED. Nạp environment sender của Kan_sim
cho board phát riêng (không nạp receiver lên board LED vì sẽ mất chương trình LED).

```powershell
cd D:\SEP\Kan_sim
python tools/acc_bridge.py --port COM7
# Hoặc đóng bridge ACC trước rồi test bằng lệnh thủ công:
python tools/simulator.py --port COM7
```

## Frame và trạng thái

Chỉ nhận standard data frame ID `0x321`, DLC 8, version byte0 = 1.
Byte1: bit0–1 turn (0 off, 1 left, 2 right, 3 hazard), bit2 reverse, bit3 brake.
Byte2–3 speed uint16 little endian theo 0.1 km/h, 0–2000. Byte4 counter;
byte5–7 và bit4–7 byte1 phải bằng 0. Frame extended/RTR/sai version/reserved/
speed bị bỏ qua, không làm mới timeout. Counter chỉ chẩn đoán, không chống replay.
Giao thức này là test riêng, không giải mã Hyundai i10 và không nối bus test vào xe.

Khởi động: UNKNOWN. Nhận hợp lệ: log `[CAR_SIG] CAN turn=... brake=... reverse=...
speed=... stopped=... counter=...`. Quá 500 ms không có frame hợp lệ: UNKNOWN,
bỏ toàn bộ override ô tô và trả về hiệu ứng nền. Không suy mất CAN thành xe dừng.
Frame hợp lệ trở lại tự khôi phục. Mất ACC/USB có thể thêm độ trễ watchdog phía
sender trước khi ngừng CAN (xem Kan_sim/docs/ACC.md).

## Hiển thị trên LED

Thứ tự ưu tiên khi power đang bật:
1. Trái/phải/hazard: hiệu ứng mũi tên vàng hiện có; nếu đồng thời phanh hoặc lùi
   thì vạch dưới cùng màu đỏ (phanh) hoặc trắng (lùi).
2. Phanh: nền đỏ, chữ BRAKE trắng.
3. Lùi: nền trắng, chữ REVERSE đen.
4. Dừng (speed = 0): chữ STOP đỏ trên nền đen.
5. Đang chạy, không có tín hiệu: hiệu ứng nền do ứng dụng chọn.

Power OFF vẫn tắt màn hình, không bị CAN bật lại. Trạng thái CAN không lưu NVS,
không đổi hiệu ứng nền đang chọn. Mất override sẽ yêu cầu vẽ lại custom canvas.
Phanh chỉ là bit bật/tắt; ga/RPM/góc lái chưa nằm trong frame test.

Serial L/R/H/S/0 và ? vẫn dùng được khi không có CAN mới. CAN mới có ưu tiên và
xóa trạng thái giả lập cũ; Serial không ghi đè khi CAN đang hợp lệ. S/0 là tắt
xi-nhan test, không phải tín hiệu xe dừng. `?` in nguồn CAN/SERIAL/UNKNOWN.

## Checklist thử trên bàn

- Sender RESET → LED STOP; SPEED 25 → trở lại hiệu ứng nền.
- LEFT / RIGHT / HAZARD / OFF → mũi tên đúng; BRAKE ON/OFF → nền đỏ/về nền.
- REVERSE ON + SPEED 5 → nền trắng; STOP với reverse OFF/brake OFF → STOP.
- Thử đồng thời hazard + brake: mũi tên và vạch đỏ.
- PAUSE sender hoặc tháo bus → log STALE và trở lại nền sau hơn 500 ms.
- Khởi động LED không có sender → không hiện STOP; BLE/WiFi vẫn dùng được.

Unit suite `test_can_signals` kiểm tra decoder, frame lỗi không sửa trạng thái,
timeout qua tràn millis và thứ tự ưu tiên; cần ESP32 để thực thi Unity:

```powershell
pio test -e esp32dev-can-test --without-uploading --without-testing
# Chạy test trên board ESP32 dự phòng, sẽ nạp firmware test:
pio test -e esp32dev-can-test --upload-port COM9 --test-port COM9
```
