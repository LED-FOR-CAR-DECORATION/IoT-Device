# Hướng dẫn phần cứng & Sơ đồ đấu nối (Hardware Schematics)

Hệ thống điện ô tô sử dụng điện áp bình 12V (hoặc lên đến 14.4V khi nổ máy). Do đó, việc thiết kế mạch nguồn hạ áp và bảo vệ cho vi điều khiển ESP32 và dải LED 5V là cực kỳ quan trọng.

---

## 1. Sơ đồ khối nguồn và tín hiệu

```
[Bình ô tô 12V / ACC] ───► [Cầu chì 5A] ───► [Mạch Hạ áp Buck 12V -> 5V (5A)]
                                                     │
               ┌─────────────────────────────────────┴────────────────────────┐
               ▼                                                              ▼
        [ESP32 DevKit (Vin 5V)]                                     [LED Strip WS2812B (5V)]
               │ (GPIO 18 - 3.3V)                                             ▲
               ▼                                                              │ (Data 5V)
    [Mạch chuyển đổi mức logic (Level Shifter 3.3V -> 5V)] ───────────────────┘
               │ (qua trở 470Ω)
```

---

## 2. Bảng chân kết nối (Pinout Matrix)

| Thiết bị ngoại vi | Chân ESP32 | Chú thích |
|---|---|---|
| **LED Matrix P5 (HUB75 DMA)** | R1: 25, G1: 26, B1: 27<br>R2: 14, G2: 12, B2: 13<br>A: 23, B: 19, C: 5, D: 17<br>LAT: 4, OE: 15, CLK: 16 | Chuẩn DMA I2S điều khiển ma trận LED RGB 64x32 |
| **Nút Dừng Khẩn Cấp (E-Stop)** | **GPIO 32** | Nút cơ khẩn cấp kéo xuống GND (Internal Pullup, ngắt LED tức thì) |
| **Microphone ADC** | **GPIO 34** | Chân Analog đọc tín hiệu âm thanh từ cảm biến mic MAX4466 / MAX9814 |
| **Nút bấm Mode** | **GPIO 0** | Nút bấm vật lý (nút Boot onboard hoặc nút bấm cơ ngoài xe nối GND) |
| **ACC Detect (Tùy chọn)** | **GPIO 35** | Tín hiệu nhận biết xe nổ máy / mở khóa điện (qua cầu phân áp hoặc optocoupler) |
| **Status LED** | **GPIO 2** | Đèn LED báo hiệu trạng thái hoạt động trên board ESP32 |

---

## 3. Sơ đồ đấu nối Nút Dừng Khẩn Cấp (E-Stop Button)

Nút dừng khẩn cấp sử dụng tiếp điểm thường mở (NO - Normally Open), tích hợp trở kéo lên nội vi `INPUT_PULLUP`:

```text
[ Chân GPIO 32 ] ────► [ Chân NO ]
                             │  (Nhấn nút ➔ Thông mạch xuống Mass)
[ Chân GND     ] ────► [ Chân C  ]

[ Chân NC      ] ────► (Bỏ trống)
```

- **Chân C (Common):** Nối vào chân `GND` của ESP32.
- **Chân NO (Normally Open):** Nối vào chân `GPIO 32` của ESP32.
- **Chân NC:** Bỏ trống.
- **Nguyên lý hoạt động:** Bình thường tiếp điểm mở, GPIO 32 ở mức `HIGH` (3.3V). Khi nhấn nút dừng khẩn cấp, chân GPIO 32 bị chập xuống `LOW` (GND), vi điều khiển bắt ngắt tức thì để xóa bộ đệm DMA và ngắt phát màn hình LED.

---

## 3. Các lưu ý bảo vệ an toàn trên ô tô

1. **Tụ lọc nguồn:** Đặt một tụ hóa $1000\mu\text{F} / 16\text{V}$ hoặc $2200\mu\text{F} / 16\text{V}$ song song tại đầu cấp nguồn $5\text{V}$ và $\text{GND}$ của dải LED để hấp thụ shock áp khi cắm/rút nguồn xe.
2. **Điện trở Data Line:** Đặt một điện trở $330\Omega$ đến $470\Omega$ giữa chân Data của ESP32 (hoặc Level Shifter) và chân Din của dải LED đầu tiên.
3. **Mức logic Data:** ESP32 xuất tín hiệu $3.3\text{V}$, trong khi WS2812B chạy chuẩn $5\text{V}$. Dù đôi khi WS2812B vẫn nhận trực tiếp $3.3\text{V}$, nhưng khi dây dài hoặc nhiệt độ trong xe thay đổi sẽ gây nhấp nháy (flickering). Khuyên dùng chip logic shifter tốc độ cao như `74AHCT12 logic chip` hoặc module chuyển đổi mức logic $3.3\text{V} \leftrightarrow 5\text{V}$.
4. **Giới hạn dòng trong Firmware:** Trong file `include/config.h`, hệ thống đã cấu hình:
   ```cpp
   FastLED.setMaxPowerInVoltsAndMilliamps(5, MAX_CURRENT_MA);
   ```
   giúp ngăn chặn dải LED tiêu thụ quá dòng làm sụt áp mạch nguồn hoặc quá nhiệt.
