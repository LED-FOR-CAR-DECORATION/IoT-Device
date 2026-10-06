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
| **LED Strip Data** | **GPIO 18** | Tín hiệu điều khiển dữ liệu LED qua Level Shifter (3.3V -> 5V) + trở 470Ω |
| **Microphone ADC** | **GPIO 34** | Chân Analog đọc tín hiệu âm thanh từ cảm biến mic MAX4466 / MAX9814 |
| **Nút bấm Mode** | **GPIO 0** | Nút bấm vật lý (nút Boot onboard hoặc nút bấm cơ ngoài xe nối GND) |
| **ACC Detect (Tùy chọn)** | **GPIO 35** | Tín hiệu nhận biết xe nổ máy / mở khóa điện (qua cầu phân áp hoặc optocoupler) |
| **Status LED** | **GPIO 2** | Đèn LED báo hiệu trạng thái hoạt động trên board ESP32 |

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
