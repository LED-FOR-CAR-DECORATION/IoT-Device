# API & Communication Protocol Specification

Tài liệu đặc tả giao thức giao tiếp giữa ứng dụng Mobile (Flutter / React Native / Native) và thiết bị IoT điều khiển LED xe ô tô.

---

## 1. Bluetooth Low Energy (BLE) Profile

* **Device Advertising Name:** `CarLED-Ambient`
* **Primary Service UUID:** `4fafc201-1fb5-459e-8fcc-c5c9c331914b`

### Danh sách Characteristics

| Tên Characteristic | UUID | Quyền (Properties) | Chức năng |
|---|---|---|---|
| **Control Characteristic** | `beb5483e-36e1-4688-b7f5-ea07361b26a8` | `WRITE`, `WRITE_WITHOUT_RESPONSE` | App gửi gói tin JSON lệnh điều khiển đến ESP32 |
| **Status Characteristic** | `1c95d5e3-d8f7-413a-bf3d-7a2e5d7be87e` | `READ`, `NOTIFY` | ESP32 phát thông báo trạng thái hiện tại về cho App |

---

## 2. Định dạng gói tin JSON (Command Payloads)

### Bật / Tắt nguồn đèn
```json
{
  "power": true
}
```

### Điều chỉnh độ sáng (0 - 255)
```json
{
  "brightness": 200
}
```

### Đổi chế độ hiệu ứng (Mode) & Tốc độ (Speed: 1 - 100)
```json
{
  "mode": 3,
  "speed": 65
}
```
*Bảng mã Mode (`mode`):*
* `0`: OFF
* `1`: STATIC_COLOR (Màu đơn sắc)
* `2`: BREATHING (Nhịp thở chuyển màu)
* `3`: RAINBOW_WAVE (Cầu vồng chuyển động)
* `4`: CHASE_FLOW (Dòng chảy đuổi màu)
* `5`: MUSIC_REACTIVE (Nháy theo nhịp điệu nhạc xe)
* `6`: CUSTOM_PRESET (Ảnh/Animation Character chiếu từ Flash SPIFFS)

> **Lưu ý hiệu năng:** Với các thao tác vuốt thời gian thực (Slider độ sáng, Color Wheel), khuyến nghị ưu tiên dùng **Giao thức Nhị phân 7-byte có Checksum** quy định tại [protocols/commands.md](file:///d:/SEP/IoT-Device/protocols/commands.md). Giao thức JSON dùng cho cấu hình và MQTT.

### Đổi màu sắc (RGB Color Picker)
```json
{
  "mode": 1,
  "color": {
    "r": 0,
    "g": 200,
    "b": 255
  }
}
```

### Điều chỉnh độ nhạy micro bắt nhạc (Sensitivity: 1 - 100)
```json
{
  "sensitivity": 75
}
```

---

## 3. Giao thức MQTT (Khi kết nối qua Wi-Fi / Cloud)

* **Topic Lệnh (Subscribe):** `car/led/command`
* **Topic Trạng thái (Publish):** `car/led/status`
* Cấu trúc payload hoàn toàn tương đồng với định dạng JSON BLE nêu trên.

---

## 4. Định dạng Gói tin Báo cáo Trạng thái (Telemetry & Events)

Thiết bị phát qua **BLE Status Characteristic (Notify)** và **MQTT Topic `car/led/status`**:

### Định kỳ Heartbeat Telemetry (Mỗi 5 giây):
```json
{
  "power": true,
  "brightness": 180,
  "speed": 60,
  "mode": 6,
  "color": {"r": 0, "g": 150, "b": 255},
  "emergency_stop": false,
  "free_heap": 182400
}
```

### Sự kiện Dừng Khẩn Cấp (E-Stop Immediate Notification):
- **Khi nhấn nút E-Stop (GPIO 32 thông mạch xuống GND):**
```json
{
  "event": "emergency_stop",
  "active": true,
  "message": "EMERGENCY STOP TRIGGERED: Matrix Blanked"
}
```
- **Khi xoay mở/nhả nút E-Stop (GPIO 32 trở lại mức HIGH):**
```json
{
  "event": "emergency_stop",
  "active": false,
  "message": "Emergency Stop Released: Resuming normal operation"
}
```

