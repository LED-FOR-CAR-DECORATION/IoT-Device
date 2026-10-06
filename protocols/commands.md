# Giao thức Lệnh Điều khiển Thiết bị (Commands Protocol)

Tài liệu này là **"Hợp đồng kỹ thuật" (Contract Specification)** quy định phương thức giao tiếp giữa **Mobile App (Flutter / React Native / Native)**, **Backend Server** và **Thiết bị LED IoT (ESP32)**.

---

## 1. Phương thức truyền thông (Transport Layers)

Thiết bị hỗ trợ nhận lệnh qua 2 kênh truyền:

| Kênh truyền | Giao thức | Địa chỉ / UUID / Topic | Phạm vi áp dụng |
|---|---|---|---|
| **Bluetooth BLE** | GATT Service (NUS Custom) | Service UUID: `4fafc201-1fb5-459e-8fcc-c5c9c331914b`<br>Write UUID: `beb5483e-36e1-4688-b7f5-ea07361b26a8`<br>Notify UUID: `1c95d5e3-d8f7-413a-bf3d-7a2e5d7be87e` | Trong cabin ô tô, không cần Internet |
| **Wi-Fi / Cloud** | MQTT over TCP (Port 1883) | Subscribe Topic: `car/led/command`<br>Publish Status Topic: `car/led/status` | Điều khiển từ xa qua 4G/Cloud |

---

## 2. Giao thức Nhị phân Tối ưu Cao (High-Speed 6-Byte Binary Protocol)

> **Khuyến nghị ưu tiên hàng đầu:** Dùng giao thức này cho Bluetooth BLE trên Mobile App khi tương tác vuốt thời gian thực (Color Picker, Brightness Slider 60 FPS).
> - Gói tin cố định đúng **6 Bytes**, vừa vặn trong 1 frame BLE tiêu chuẩn (20 bytes MTU mặc định của Android).
> - Giảm 90% chi phí CPU parsing của ESP32, độ trễ phản hồi < 5ms, triệt tiêu hoàn toàn hiện tượng nghẽn hàng đợi GATT và trễ hình khi lướt ngón tay.

### 2.1. Cấu trúc Khung truyền (Frame Format)

| Byte | Tên trường | Giá trị | Ý nghĩa |
|---|---|---|---|
| Byte 0 | **Header (SOF)** | `0xAA` | Bắt đầu khung gói tin |
| Byte 1 | **Opcode (CMD)** | `0x01` ~ `0x06` | Mã lệnh chức năng |
| Byte 2 | **Param 1 (P1)** | `0x00` ~ `0xFF` | Tham số 1 (Power / Brightness / Speed / Mode / Red / Sens) |
| Byte 3 | **Param 2 (P2)** | `0x00` ~ `0xFF` | Tham số 2 (Green trong lệnh RGB, các lệnh khác đặt `0x00`) |
| Byte 4 | **Param 3 (P3)** | `0x00` ~ `0xFF` | Tham số 3 (Blue trong lệnh RGB, các lệnh khác đặt `0x00`) |
| Byte 5 | **Tail (EOF)** | `0x55` | Kết thúc khung gói tin |

### 2.2. Bảng mã Opcode chi tiết

| Opcode | Chức năng | Param 1 (P1) | Param 2 (P2) | Param 3 (P3) | Ví dụ Hex |
|---|---|---|---|---|---|
| `0x01` | Power On / Off | `0x01` (ON) / `0x00` (OFF) | `0x00` | `0x00` | Bật: `AA 01 01 00 00 55`<br>Tắt: `AA 01 00 00 00 55` |
| `0x02` | Brightness (Độ sáng) | `0x00` ~ `0xFF` (0 - 255) | `0x00` | `0x00` | Đặt sáng 180: `AA 02 B4 00 00 55` |
| `0x03` | Speed (Tốc độ) | `0x01` ~ `0x64` (1 - 100) | `0x00` | `0x00` | Tốc độ 60: `AA 03 3C 00 00 55` |
| `0x04` | Mode (Hiệu ứng) | `0x00` ~ `0x05` (Mode enum) | `0x00` | `0x00` | Cầu vồng (3): `AA 04 03 00 00 55` |
| `0x05` | Set RGB Color | Red (`0` - `255`) | Green (`0` - `255`) | Blue (`0` - `255`) | Đỏ tinh khiết: `AA 05 FF 00 00 55`<br>Xanh Ice Blue: `AA 05 00 96 FF 55` |
| `0x06` | Mic Sensitivity | `0x01` ~ `0x64` (1 - 100) | `0x00` | `0x00` | Độ nhạy 80: `AA 06 50 00 00 55` |

---

## 3. Cấu trúc Gói tin Lệnh (JSON Format - Dùng cho MQTT / Preset)

Mọi gói tin cấu hình hoặc gửi từ MQTT Broker / Cloud xuống ESP32 đều ở định dạng chuỗi **JSON** nén (minify). Firmware tự động nhận diện dựa vào Header byte hoặc cú pháp JSON.

### 3.1. Bật / Tắt nguồn (Power Toggle)
* **Mô tả:** Bật hoặc tắt phát sáng toàn bộ dải LED.
* **Payload:**
```json
{
  "power": true
}
```
* **Giá trị:** `true` (Bật sáng), `false` (Tắt sáng).

---

### 2.2. Điều chỉnh độ sáng (Brightness)
* **Mô tả:** Thay đổi độ sáng tổng của hệ thống LED.
* **Payload:**
```json
{
  "brightness": 180
}
```
* **Dải giá trị:** `0` đến `255` (Khuyên dùng: 50 – 200 để chống chói mắt vào ban đêm trong cabin).

---

### 2.3. Đổi Chế độ Hiệu ứng & Tốc độ (Effect Mode & Speed)
* **Mô tả:** Chuyển đổi giữa các hiệu ứng động có sẵn trong firmware.
* **Payload:**
```json
{
  "mode": 3,
  "speed": 60
}
```
* **Bảng mã hiệu ứng (`mode`):**
  - `0`: **OFF** (Tắt)
  - `1`: **STATIC_COLOR** (Màu đơn sắc tĩnh)
  - `2`: **BREATHING** (Nhịp thở chuyển màu mượt)
  - `3`: **RAINBOW_WAVE** (Cầu vồng sóng chuyển động)
  - `4`: **CHASE_FLOW** (Hiệu ứng dòng chảy đuổi màu)
  - `5`: **MUSIC_REACTIVE** (Nháy vũ đạo theo cảm biến âm thanh)
* **Tốc độ (`speed`):** `1` (Rất chậm) đến `100` (Rất nhanh).

---

### 2.4. Đổi màu sắc (RGB Color Picker)
* **Mô tả:** Đổi màu đơn sắc cho chế độ Static Color hoặc Breathing.
* **Payload:**
```json
{
  "mode": 1,
  "color": {
    "r": 255,
    "g": 50,
    "b": 100
  }
}
```
* **Dải màu:** `r`, `g`, `b` nhận giá trị từ `0` đến `255`.

---

### 2.5. Độ nhạy Cảm ứng Nhạc (Music Sensitivity)
* **Mô tả:** Điều chỉnh độ nhạy của micro cảm biến âm thanh khi bật Mode 5.
* **Payload:**
```json
{
  "sensitivity": 80
}
```
* **Dải giá trị:** `1` (Kém nhạy, chỉ nhận tiếng bass lớn) đến `100` (Cực nhạy, nhận tiếng thì thầm).

---

## 3. Gói tin Phản hồi Trạng thái từ Thiết bị (Device Status Telemetry)

Mỗi khi nhận lệnh thành công hoặc định kỳ mỗi 5 giây, ESP32 sẽ phát ngược lại gói tin JSON báo cáo trạng thái lên cổng Notify BLE hoặc topic MQTT `car/led/status`:

```json
{
  "power": true,
  "brightness": 180,
  "mode": 3,
  "speed": 60,
  "color": {
    "r": 255,
    "g": 50,
    "b": 100
  },
  "sensitivity": 80,
  "acc_on": true,
  "free_heap": 182400
}
```

* `acc_on`: Trạng thái khóa điện xe hơi (`true`: Xe đang nổ máy/bật khóa, `false`: Tắt máy).
* `free_heap`: Dung lượng RAM còn trống của ESP32 (dùng theo dõi độ ổn định).
