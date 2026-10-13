# Sơ đồ Nguyên lý Đấu nối Phần cứng (Hardware Wiring Schematic)

Tài liệu hướng dẫn kết nối các khối phần cứng trong hệ thống Smart Car Ambient LED Lighting.

---

## 1. Sơ đồ Chân Vi điều khiển ESP32 (Pinout Mapping)

| Tên Ngoại vi | Chân ESP32 | Loại tín hiệu | Điện áp | Ghi chú kỹ thuật |
|---|---|---|---|---|
| **LED Matrix P5 (HUB75 DMA)** | R1: 25, G1: 26, B1: 27<br>R2: 14, G2: 12, B2: 13<br>A: 23, B: 19, C: 5, D: 17<br>LAT: 4, OE: 15, CLK: 16 | Digital Output | 3.3V -> 5V | Chuẩn DMA I2S điều khiển ma trận LED RGB 64x32 |
| **Nút Dừng Khẩn Cấp (E-Stop)** | **GPIO 32** | Digital Input | 3.3V | Nút cơ NO nối GND (Internal Pullup, ngắt LED tức thì) |
| **Microphone ADC** | **GPIO 34** | Analog Input | 0 - 3.3V | Từ module MAX4466 / MAX9814 (chân VOUT) |
| **Nút bấm Mode** | **GPIO 0** | Digital Input | 3.3V | Nút nhấn nhả nối xuống GND (Internal Pullup) |
| **Tín hiệu ACC** | **GPIO 35** | Digital Input | 3.3V | Tín hiệu 12V chìa khóa xe qua Optocoupler PC817 |
| **Nguồn cấp ESP32** | **VIN / 5V** | Power In | 5.0V | Từ mạch Buck Converter 12V -> 5V |
| **Mass chung** | **GND** | Ground | 0V | Nối chung toàn bộ hệ thống |

---

## 2. Sơ đồ Khối Toàn hệ thống (Block Diagram)

```text
[ Bình Ắc quy Ô tô 12V-14V ]
           │
      [ Cầu chì 10A ]
           │
           ▼
[ Mạch Hạ áp DC-DC Buck 12V -> 5V 10A ]
           │
           ├────────────────────────────┬─────────────────────────────┐
           ▼ (5V)                       ▼ (5V)                        ▼ (5V)
    [ Dải LED WS2812B ]          [ IC Logic Shifter ]          [ Chân VIN ESP32 ]
           ▲                            ▲                             │
           │ (Data 5V)                  │ (Data 3.3V)                 │
           └────────────────── [ GPIO 18 qua 74HCT125 ] ◄─────────────┘
                                                                      │
 [ Micro Âm thanh MAX4466 ] ────► (Tín hiệu Analog 0-3V) ────► [ GPIO 34 ]
                                                                      │
 [ Tín hiệu Khóa xe ACC 12V ] ──► [ Cách ly quang PC817 ] ───► [ GPIO 35 ]
                                                                       │
 [ Nút bấm Chuyển Mode ] ───────► (Nhấn xuống Mass 0V) ──────► [ GPIO 0 ]
```

---

## 3. Các lưu ý kỹ thuật an toàn trên Ô tô:
1. **Đệm mức logic (Logic Level Shifter):** ESP32 phát tín hiệu 3.3V, dải LED WS2812B cần tín hiệu 5V. Luôn dùng IC đệm (như `74HCT125` hoặc `74AHCT125`) và một điện trở `330Ω - 470Ω` trên đường tín hiệu Data để khử xung dội ngược.
2. **Tụ lọc nguồn:** Đặt 1 tụ hóa `1000µF / 16V` ngay tại đầu vào nguồn 5V của dải LED để hấp thụ sụt áp đột ngột khi toàn bộ LED bật sáng màu trắng.
3. **Cách ly tín hiệu khóa điện (ACC):** Điện áp trên ô tô khi nổ máy có thể sinh gai áp lên tới 30V-40V (Load Dump). Bắt buộc dùng mạch cách ly quang **Optocoupler PC817** giữa đường ACC 12V và chân GPIO 35 của ESP32.
