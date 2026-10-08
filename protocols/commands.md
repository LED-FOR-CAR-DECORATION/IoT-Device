# Giao thức Lệnh Điều khiển Thiết bị (Commands Protocol)

Tài liệu này là **"Hợp đồng kỹ thuật" (Contract Specification)** quy định phương thức giao tiếp giữa **Mobile App (Flutter / React Native / Kotlin Native)**, **Backend Server** và **Thiết bị LED IoT (ESP32)**.

---

## 1. Phương thức truyền thông (Transport Layers)

Thiết bị hỗ trợ nhận lệnh qua 2 kênh truyền:

| Kênh truyền | Giao thức | Địa chỉ / UUID / Topic | Đặc tính kỹ thuật | Phạm vi áp dụng |
|---|---|---|---|---|
| **Bluetooth BLE** | GATT Service (Custom) | Service UUID: `4fafc201-1fb5-459e-8fcc-c5c9c331914b`<br>Control UUID: `beb5483e-36e1-4688-b7f5-ea07361b26a8`<br>Status UUID: `1c95d5e3-d8f7-413a-bf3d-7a2e5d7be87e` | - Properties: `WRITE`, `WRITE_NR`<br>- MTU: 256 bytes (Negotiated)<br>- Connection Interval: 15ms – 30ms | Trong cabin xe, không cần Internet |
| **Wi-Fi / Cloud** | MQTT over TCP (Port 1883) | Subscribe Topic: `car/led/command`<br>Publish Status Topic: `car/led/status` | - QoS 0 / 1<br>- Chuỗi JSON tiêu chuẩn | Điều khiển từ xa qua 4G/Cloud |

---

## 2. Giao thức Nhị phân (Binary Protocol - Tối ưu BLE)

> **Mục đích:** Sử dụng cho các thao tác kéo trượt thời gian thực (Color Picker, Brightness Slider, Live Canvas) nhằm loại bỏ chi phí serialize/parse chuỗi, tránh phân mảnh gói tin và đồng bộ mượt mà với Connection Interval của BLE.

### 2.1. Cấu trúc Khung truyền Chuẩn (7-Byte Frame with XOR Checksum)

Để đảm bảo toàn vẹn dữ liệu ở tầng ứng dụng, khung lệnh chuẩn bổ sung trường **XOR Checksum**:

| Byte | Tên trường | Kiểu | Mô tả |
|---|---|---|---|
| Byte 0 | **Header (SOF)** | `uint8_t` | Luôn cố định là `0xAA` |
| Byte 1 | **Opcode (CMD)** | `uint8_t` | Mã lệnh điều khiển (`0x01` ~ `0x14`) |
| Byte 2 | **Param 1 (P1)** | `uint8_t` | Tham số 1 (Power / Brightness / Speed / Mode / Red) |
| Byte 3 | **Param 2 (P2)** | `uint8_t` | Tham số 2 (Green trong RGB, hoặc `0x00`) |
| Byte 4 | **Param 3 (P3)** | `uint8_t` | Tham số 3 (Blue trong RGB, hoặc `0x00`) |
| Byte 5 | **Checksum (XOR)** | `uint8_t` | `P1 ^ P2 ^ P3 ^ CMD` (XOR từ Byte 1 đến Byte 4) |
| Byte 6 | **Tail (EOF)** | `uint8_t` | Luôn cố định là `0x55` |

> **Tương thích ngược (Backward Compatibility):** ESP32 đồng thời chấp nhận khung 6 bytes cũ (`AA | cmd | p1 | p2 | p3 | 55`) cho các client chưa nâng cấp checksum.

### 2.2. Bảng mã Opcode chi tiết

| Opcode | Chức năng | Param 1 (P1) | Param 2 (P2) | Param 3 (P3) | Ví dụ Hex (Kèm Checksum) |
|---|---|---|---|---|---|
| `0x01` | Power On / Off | `0x01` (ON) / `0x00` (OFF) | `0x00` | `0x00` | Bật: `AA 01 01 00 00 00 55`<br>Tắt: `AA 01 00 00 00 01 55` |
| `0x02` | Brightness (Độ sáng) | `0x00` ~ `0xFF` (0 - 255) | `0x00` | `0x00` | Sáng 180 (0xB4): `AA 02 B4 00 00 B6 55` |
| `0x03` | Speed (Tốc độ) | `0x01` ~ `0x64` (1 - 100) | `0x00` | `0x00` | Tốc độ 60 (0x3C): `AA 03 3C 00 00 3F 55` |
| `0x04` | Mode (Hiệu ứng) | `0x00` ~ `0x06` (Mode enum) | `0x00` | `0x00` | Cầu vồng (3): `AA 04 03 00 00 07 55` |
| `0x05` | Set RGB Color | Red (`0` - `255`) | Green (`0` - `255`) | Blue (`0` - `255`) | Đỏ: `AA 05 FF 00 00 FA 55`<br>Ice Blue (0, 150, 255): `AA 05 00 96 FF 6C 55` |
| `0x06` | Mic Sensitivity | `0x01` ~ `0x64` (1 - 100) | `0x00` | `0x00` | Độ nhạy 80 (0x50): `AA 06 50 00 00 56 55` |
| `0x10` | **Projection Start** | `frameCount` | `width` (64) | `height` (32) | Khung 8/9 bytes: Bắt đầu nạp Animation vào Flash SPIFFS |
| `0x11` | **Projection Chunk** | `chunkIdxH` | `chunkIdxL` | `dataLen` | Nạp từng khối dữ liệu RGB565 |
| `0x13` | **Draw Live Pixel** | `x` (0..63) | `y` (0..31) | `r` | Vẽ 1 điểm ảnh đơn lẻ |
| `0x14` | **Draw Batch Points** | `pointCount` | *(dữ liệu điểm)* | *(dữ liệu điểm)* | Gom nhiều điểm chạm vào 1 gói (xem mục 2.3) |
| `0x15` | **Canvas Clear (Blank Canvas)** | `0x00` | `0x00` | `0x00` | Xóa sạch bảng LED về màn hình đen tuyền để bắt đầu vẽ |

---

### 2.3. Lệnh Gom Điểm Chạm (Batch Drawing - Opcode 0x14)

Khi người dùng vẽ trực tiếp trên màn hình điện thoại (tần số quét cảm ứng 60Hz – 120Hz), **tuyệt đối không gửi từng gói 0x13 cho mỗi điểm chạm**, vì sẽ gây nghẽn hàng đợi BLE.

App gom các điểm chạm trong vòng 30ms – 40ms thành một gói gộp:

```
[0xAA] [0x14] [count] [x0, y0, r0, g0, b0] [x1, y1, r1, g1, b1] ... [checksum] [0x55]
```
- `count`: Số lượng điểm ảnh trong gói (ví dụ 1 đến 15 điểm).
- Mỗi điểm gồm 5 bytes: `x`, `y`, `r`, `g`, `b`.
- `checksum`: XOR của toàn bộ các byte từ Opcode `0x14` đến hết điểm cuối cùng.

---

## 3. Giao thức Chuỗi JSON (JSON Format - Dùng cho Cấu hình & MQTT)

Dùng cho các lệnh có cấu trúc phức tạp: cấu hình Wi-Fi, đặt tên thiết bị, đổi preset hoặc khi điều khiển qua Cloud/MQTT.

### 3.1. Cơ chế Ghép gói Phân mảnh (MTU Chunk Assembly trên ESP32)

- **Vấn đề thực tế:** Mặc định của BLE ATT chỉ cho phép payload tối đa **20 bytes** nếu client chưa thỏa thuận MTU. Chuỗi JSON lớn (Wi-Fi SSID + Password, OTA, telemetry) sẽ bị hệ điều hành Android/iOS tự động cắt thành nhiều lần ghi GATT.
- **Giải pháp trên ESP32 Firmware:**
  1. ESP32 thiết lập `NimBLEDevice::setMTU(256)`.
  2. Tích hợp bộ đệm tích lũy dòng ký tự (`jsonRxBuffer`). ESP32 tự động gom các mảnh cho tới khi chuỗi JSON đạt trạng thái hoàn chỉnh (cân bằng dấu `{...}` hoặc kết thúc bằng ký tự xuống dòng `\n`).
  3. Cơ chế Garbage Collector Timeout: Nếu sau **1000ms** gói tin JSON bị dở dang do đứt kết nối, ESP32 tự động giải phóng bộ đệm để chống rò rỉ RAM.

### 3.2. Định dạng Lệnh JSON Mẫu

#### Bật / Tắt nguồn:
```json
{"power": true}
```

#### Đổi độ sáng & Hiệu ứng:
```json
{
  "brightness": 180,
  "mode": 3,
  "speed": 60
}
```

#### Bảng mã `mode`:
- `0`: OFF
- `1`: STATIC_COLOR
- `2`: BREATHING
- `3`: RAINBOW_WAVE
- `4`: CHASE_FLOW
- `5`: MUSIC_REACTIVE
- `6`: CUSTOM_PRESET (Ảnh/Animation nạp từ Flash SPIFFS)

#### Nạp / Quản lý Animation (Flow 2):
```json
{"command": "reload_projection"}
```
```json
{"command": "clear_projection"}
```

---

## 4. Báo cáo Trạng thái Thiết bị (Status Telemetry)

Phát qua BLE Status Characteristic (Notify) hoặc MQTT Topic `car/led/status`:

```json
{
  "power": true,
  "brightness": 180,
  "mode": 6,
  "speed": 60,
  "color": {"r": 0, "g": 150, "b": 255},
  "free_heap": 182400
}
```

---

## 5. Quy chuẩn Kỹ thuật Thực tế cho Mobile App (Frontend Guidance)

1. **Tốc độ gửi lệnh (Throttling):**
   - Tốc độ truyền BLE bị chi phối bởi *Connection Interval* (thông thường 15ms – 30ms trên Android/iOS).
   - Tần suất gửi thanh trượt (slider / color wheel) nên đặt ở mức **20 – 30 lệnh/giây** (chu kỳ 33ms – 50ms). Không bắn 60 – 120 lệnh/giây vì các lệnh dư thừa chỉ bị xếp hàng hoặc drop ở tầng radio.
2. **Kiểu ghi GATT (Write Type):**
   - Với lệnh thanh trượt tần suất cao: Bắt buộc dùng `WRITE_WITHOUT_RESPONSE` (`WRITE_NR`).
   - Control Characteristic của ESP32 đã được cấu hình hỗ trợ cả `WRITE` và `WRITE_NR`.
3. **Phối hợp Lệnh trên Cùng 1 Characteristic:**
   - Khi người dùng đang kéo slider đổi màu (bắn khung nhị phân liên tục), App cần chặn (debounce) các lệnh JSON nặng để tránh làm phân mảnh bộ đệm nhận diện của ESP32.
