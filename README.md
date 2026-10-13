# Smart Car Ambient LED Lighting - IoT Device Firmware

Repository chứa mã nguồn Firmware cho hệ thống điều khiển LED trang trí nội thất ô tô thông minh (Car Ambient Lighting). Dự án chạy trên nền tảng vi điều khiển **ESP32**, xây dựng với công cụ quản lý dự án **PlatformIO** theo chuẩn kiến trúc hướng đối tượng & module hoá phân tầng.

---

## 🌟 Tính năng nổi bật

- **Kiến trúc Đa nhân (Dual-Core FreeRTOS):**
  - **Core 1:** Render hiệu ứng LED Matrix P5 mượt mà đạt chuẩn 60 FPS (I2S DMA).
  - **Core 0:** Điều phối Bluetooth BLE, Wi-Fi, MQTT, OTA.
- **Hiệu ứng ma trận LED đa dạng (Effects):**
  - Đơn sắc (Static Colors).
  - Nhịp thở (Breathing fade).
  - Sóng cầu vồng chuyển động (Rainbow Wave).
  - Dòng chảy ánh sáng (Chase Flow).
- **Kết nối thông minh:**
  - **Bluetooth Low Energy (BLE):** Điều khiển trực tiếp từ ứng dụng Smartphone không cần Router/Internet.
  - **Wi-Fi & MQTT:** Hỗ trợ đồng bộ đám mây và cập nhật phần mềm từ xa qua OTA (Over-The-Air).
- **Dừng khẩn cấp phần cứng (Hardware Emergency Stop - E-Stop):** Tích hợp nút ngắt khẩn cấp trên GPIO 32, lập tức blank màn hình LED P5 (< 16ms), đồng thời bắn event cảnh báo thời gian thực qua BLE Status Notification, MQTT và Web Suite.
- **Bộ nhớ lưu trạng thái (NVS Storage):** Tự động lưu màu sắc, độ sáng, chế độ hoạt động và tự phục hồi khi khởi động lại.

---

## 📁 Cấu trúc thư mục dự án

```text
IoT-Device/
├── .github/
│   └── workflows/
│       └── build.yml               # Tự động hóa CI Build trên GitHub Actions
├── docs/                           # Tài liệu thiết kế & kỹ thuật
│   ├── architecture.md             # Sơ đồ kiến trúc phần mềm & FreeRTOS
│   ├── hardware_schematic.md       # Sơ đồ kết nối phần cứng HUB75
│   └── api_protocol.md             # Giao thức BLE GATT & định dạng JSON
├── include/                        # File cấu hình & Headers dùng chung
│   ├── config.h                    # Hằng số cấu hình hệ thống (Matrix 64x32, BLE UUID,...)
│   ├── pin_definitions.h          # Định nghĩa chân kết nối HUB75 DMA của ESP32
│   ├── secrets.example.h           # File mẫu chứa thông tin nhạy cảm (WiFi, MQTT)
│   └── types.h                     # Enums và Data Structs (RGBColor, EffectMode,...)
├── src/                            # Mã nguồn chính (Source Code)
│   ├── core/                       # Lõi hệ thống & quản lý trạng thái
│   │   └── StateManager.h/.cpp     # Đọc/ghi cấu hình vào bộ nhớ NVS Flash
│   ├── drivers/                    # Trình điều khiển phần cứng ngoại vi
│   │   └── LedDriver.h/.cpp        # Driver điều khiển LED Matrix P5 (HUB75 I2S DMA)
│   ├── effects/                    # Module các thuật toán hiệu ứng chiếu sáng
│   │   ├── IEffect.h               # Interface chuẩn cho các hiệu ứng
│   │   ├── EffectManager.h/.cpp    # Quản lý chuyển đổi và render hiệu ứng
│   │   ├── StaticColorEffect.h/.cpp# Hiệu ứng màu tĩnh
│   │   ├── BreathingEffect.h/.cpp  # Hiệu ứng nhịp thở
│   │   ├── RainbowEffect.h/.cpp    # Hiệu ứng cầu vồng
│   │   └── ChaseEffect.h/.cpp      # Hiệu ứng đuổi màu
│   ├── network/                    # Giao tiếp mạng & kết nối
│   │   ├── BleManager.h/.cpp       # GATT Server Bluetooth Low Energy
│   │   ├── WifiManager.h/.cpp      # Quản lý kết nối Wi-Fi & Reconnect
│   │   ├── MqttManager.h/.cpp      # Giao thức MQTT cho Cloud / Telemetry
│   │   └── OtaManager.h/.cpp       # Nâng cấp firmware qua mạng (ArduinoOTA)
│   ├── utils/                      # Tiện ích bổ trợ
│   │   ├── Logger.h/.cpp           # Hệ thống log màu theo thời gian thực
│   │   └── ColorUtils.h            # Các hàm biến đổi không gian màu (HSV <-> RGB)
│   └── main.cpp                    # Điểm khởi chạy chính & khởi tạo tác vụ FreeRTOS
├── tools/                          # Công cụ kiểm thử Web & nạp ảnh
│   └── ble_image_importer.html     # Web BLE Suite (Chiếu ảnh, Bút vẽ Live, Bảng đèn, Telemetry)
├── lib/                            # Thư viện cục bộ (Local Libraries)
├── test/                           # Bộ kiểm thử đơn vị (Unit Tests)
├── .gitignore                      # Cấu hình bỏ qua file rác, file build và secrets
├── platformio.ini                  # File cấu hình môi trường biên dịch PlatformIO
└── README.md                       # Tài liệu hướng dẫn sử dụng
```

---

## 🚀 Hướng dẫn cài đặt & Biên dịch

### 1. Chuẩn bị môi trường
- Đã cài đặt [VS Code](https://code.visualstudio.com/) cùng tiện ích mở rộng **PlatformIO IDE**.
- Hoặc sử dụng CLI PlatformIO (`pio`).

### 2. Thiết lập thông tin bí mật (Secrets)
Tạo file `include/secrets.h` từ file mẫu:
```bash
cp include/secrets.example.h include/secrets.h
```
Điền thông tin Wi-Fi và MQTT của bạn vào file `include/secrets.h`.

### 3. Biên dịch và nạp chương trình (Build & Flash)
- **Biên dịch mã nguồn (Build):**
  ```bash
  pio run
  ```
- **Nạp vào ESP32 qua cổng COM (Upload):**
  ```bash
  pio run --target upload
  ```
- **Mở Serial Monitor kiểm tra Log:**
  ```bash
  pio device monitor
  ```
  *(Tốc độ Baudrate mặc định: `115200`)*

---

## 🛠️ Sơ đồ chân mặc định (Pinout)

| Ngoại vi | Chân ESP32 | Mô tả |
|---|---|---|
| **LED Matrix P5 (HUB75 DMA)** | R1: 25, G1: 26, B1: 27, R2: 14, G2: 12, B2: 13, A: 23, B: 19, C: 5, D: 17, LAT: 4, OE: 15, CLK: 16 | Chuẩn DMA I2S điều khiển ma trận LED RGB 64x32 |
| **Nút Dừng Khẩn Cấp (E-Stop)** | **GPIO 32** | Nút cơ khẩn cấp kéo xuống GND (Internal Pullup, ngắt LED tức thì) |
| **Microphone ADC** | **GPIO 34** | Cảm biến âm thanh (MAX4466 / MAX9814) nháy theo nhạc |
| **Nút bấm chuyển Mode** | **GPIO 0** | Nút Boot hoặc nút bấm cơ ngoài xe |
| **Tín hiệu ACC** | **GPIO 35** | Nhận biết trạng thái khóa điện ô tô |

Chi tiết sơ đồ lắp đặt mạch an toàn ô tô xem tại: [`docs/hardware_schematic.md`](docs/hardware_schematic.md).

---

## 📱 Kết nối với Mobile App

Xem tài liệu đặc tả các cổng BLE Characteristic và cấu trúc gói tin JSON tại: [`docs/api_protocol.md`](docs/api_protocol.md).
