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

## 2. Cấu trúc Gói tin Lệnh (JSON Format)

Mọi gói tin gửi từ Mobile App xuống ESP32 đều ở định dạng chuỗi **JSON** nén (minify).

### 2.1. Bật / Tắt nguồn (Power Toggle)
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
