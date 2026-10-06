# Smart Car Ambient LED Lighting - IoT Device Firmware

Repository chứa mã nguồn Firmware cho hệ thống điều khiển LED trang trí nội thất ô tô thông minh (Car Ambient Lighting). Dự án chạy trên nền tảng vi điều khiển **ESP32**, xây dựng với công cụ quản lý dự án **PlatformIO** theo chuẩn kiến trúc hướng đối tượng & module hoá phân tầng.

---

## 🌟 Tính năng nổi bật

- **Kiến trúc Đa nhân (Dual-Core FreeRTOS):**
  - **Core 1:** Render hiệu ứng LED mượt mà đạt chuẩn 60 FPS, không bị ảnh hưởng bởi tải mạng.
  - **Core 0:** Điều phối Bluetooth BLE, Wi-Fi, MQTT, cảm biến micro và nút bấm vật lý.
- **Hiệu ứng ánh sáng đa dạng (Effects):**
  - Đơn sắc (Static Colors & Ambient Palettes).
  - Nhịp thở (Breathing fade).
  - Sóng cầu vồng chuyển động (Rainbow Wave).
  - Dòng chảy ánh sáng (Chase Flow).
  - Cảm ứng âm nhạc (Music Reactive Sync qua micro analog/ADC).
- **Kết nối thông minh:**
  - **Bluetooth Low Energy (BLE):** Điều khiển trực tiếp từ ứng dụng Smartphone trong cabin mà không cần mạng Internet/Router.
  - **Wi-Fi & MQTT:** Hỗ trợ đồng bộ đám mây và cập nhật phần mềm từ xa qua OTA (Over-The-Air).
- **Bộ nhớ lưu trạng thái (NVS Storage):** Tự động lưu màu sắc, độ sáng, chế độ khi xe tắt máy (ACC ngắt) và tự phục hồi khi mở khóa xe.
- **An toàn điện ô tô (Automotive Safety):** Tích hợp thuật toán bảo vệ giới hạn dòng điện tối đa của FastLED, tránh sụt áp nguồn và quá nhiệt dải LED.

---

## 📁 Cấu trúc thư mục dự án

```text
IoT-Device/
├── .github/
│   └── workflows/
│       └── build.yml               # Tự động hóa CI Build trên GitHub Actions
├── docs/                           # Tài liệu thiết kế & kỹ thuật
│   ├── architecture.md             # Sơ đồ kiến trúc phần mềm & FreeRTOS
│   ├── hardware_schematic.md       # Sơ đồ đấu nối mạch điện ô tô 12V -> 5V
│   └── api_protocol.md             # Giao thức BLE GATT & định dạng JSON
├── include/                        # File cấu hình & Headers dùng chung
│   ├── config.h                    # Hằng số cấu hình hệ thống (số LED, BLE UUID,...)
│   ├── pin_definitions.h          # Định nghĩa chân GPIO của vi điều khiển ESP32
│   ├── secrets.example.h           # File mẫu chứa thông tin nhạy cảm (WiFi, MQTT)
│   └── types.h                     # Enums và Data Structs (RGBColor, EffectMode,...)
├── src/                            # Mã nguồn chính (Source Code)
│   ├── core/                       # Lõi hệ thống & quản lý trạng thái
│   │   ├── StateManager.h/.cpp     # Đọc/ghi cấu hình vào bộ nhớ NVS Flash
│   │   └── PowerManager.h/.cpp     # Quản lý nguồn ACC và tín hiệu cửa xe
│   ├── drivers/                    # Trình điều khiển phần cứng ngoại vi
│   │   ├── LedDriver.h/.cpp        # Driver điều khiển LED dải WS2812B (FastLED)
│   │   ├── AudioSensor.h/.cpp      # Driver đọc cảm biến âm thanh (ADC)
│   │   └── ButtonDriver.h/.cpp     # Driver nút bấm chuyển chế độ chống rung (Debounce)
│   ├── effects/                    # Module các thuật toán hiệu ứng chiếu sáng
│   │   ├── IEffect.h               # Interface chuẩn cho các hiệu ứng
│   │   ├── EffectManager.h/.cpp    # Quản lý chuyển đổi và render hiệu ứng
│   │   ├── StaticColorEffect.h/.cpp# Hiệu ứng màu tĩnh
│   │   ├── BreathingEffect.h/.cpp  # Hiệu ứng nhịp thở
│   │   ├── RainbowEffect.h/.cpp    # Hiệu ứng cầu vồng
│   │   ├── ChaseEffect.h/.cpp      # Hiệu ứng đuổi màu
│   │   └── MusicSyncEffect.h/.cpp  # Hiệu ứng nhảy theo nhạc xe
│   ├── network/                    # Giao tiếp mạng & kết nối
│   │   ├── BleManager.h/.cpp       # GATT Server Bluetooth Low Energy
│   │   ├── WifiManager.h/.cpp      # Quản lý kết nối Wi-Fi & Reconnect
│   │   ├── MqttManager.h/.cpp      # Giao thức MQTT cho Cloud / Telemetry
│   │   └── OtaManager.h/.cpp       # Nâng cấp firmware qua mạng (ArduinoOTA)
│   ├── utils/                      # Tiện ích bổ trợ
│   │   ├── Logger.h/.cpp           # Hệ thống log màu theo thời gian thực
│   │   └── ColorUtils.h            # Các hàm biến đổi không gian màu (HSV <-> RGB)
│   └── main.cpp                    # Điểm khởi chạy chính & khởi tạo tác vụ FreeRTOS
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
| **LED Data** | **GPIO 18** | Tín hiệu điều khiển LED qua Level Shifter (3.3V -> 5V) |
| **Microphone ADC** | **GPIO 34** | Cảm biến âm thanh (MAX4466 / MAX9814) nháy theo nhạc |
| **Nút bấm chuyển Mode** | **GPIO 0** | Nút Boot hoặc nút bấm cơ ngoài xe |
| **Tín hiệu ACC** | **GPIO 35** | Nhận biết trạng thái khóa điện ô tô |

Chi tiết sơ đồ lắp đặt mạch an toàn ô tô xem tại: [`docs/hardware_schematic.md`](docs/hardware_schematic.md).

---

## 📱 Kết nối với Mobile App

Xem tài liệu đặc tả các cổng BLE Characteristic và cấu trúc gói tin JSON tại: [`docs/api_protocol.md`](docs/api_protocol.md).
