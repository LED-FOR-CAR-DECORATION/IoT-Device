# Sơ đồ Nguyên lý Đấu nối Phần cứng Thực tế (Workbench / Development Setup)

Tài liệu hướng dẫn kết nối các khối phần cứng trong hệ thống hiển thị LED Ô tô trên bàn thử nghiệm (Workbench Setup).

---

## 1. Sơ đồ Chân Vi điều khiển ESP32 (Pinout Mapping)

| Tên Ngoại vi | Chân ESP32 | Loại tín hiệu | Điện áp | Ghi chú kỹ thuật |
|---|---|---|---|---|
| **Dữ liệu màu RGB Trên (Row 0-15)** | **R1 (25), G1 (26), B1 (27)** | Digital Output | 3.3V Logic | Cấp trực tiếp vào cổng HUB75 của tấm P5 |
| **Dữ liệu màu RGB Dưới (Row 16-31)** | **R2 (14), G2 (12), B2 (13)** | Digital Output | 3.3V Logic | Cấp trực tiếp vào cổng HUB75 của tấm P5 |
| **Chọn dòng quét (Row Select)** | **A (23), B (19), C (5), D (17)** | Digital Output | 3.3V Logic | Quét ma trận tỉ lệ 1/16 scan |
| **Tín hiệu đồng bộ & Chốt** | **CLK (16), LAT (4), OE (15)** | Digital Output | 3.3V Logic | Xung Clock, Chốt dòng (Latch), Bật xuất hình (OE) |
| **Nút Khẩn Cấp LA16-A (E-Stop)** | **GPIO 32** | Digital Input | 3.3V | Chân `NO` vào GPIO 32, chân `C` vào GND (Pull-up nội) |
| **Cổng Nạp & Giao tiếp PC** | **Cổng Micro-USB / Type-C** | Power & UART | 5.0V USB | Cắm vào Laptop để nạp code và giả lập lệnh Serial |
| **Mass chung (Common Ground)** | **GND** | Ground | 0V | Nối dây Mass chung giữa ESP32 và Nguồn tổ ong |

> **Ghi chú:** Hệ thống **không sử dụng** module micro âm thanh và không tích hợp các tính năng liên quan đến âm thanh.

---

## 2. Sơ đồ Khối Toàn Hệ Thống (System Block Diagram)

```text
       [ Điện lưới AC 220V ]
                 │
                 ▼ (L / N / E)
   ┌───────────────────────────┐
   │    NGUỒN TỔ ONG 5V 20A    │
   └─────────────┬─────────────┘
                 │
                 ├─────────────────── [ V+ (+5V) ] ────────────────────┐
                 │                                                     │
                 │ ┌───────────────── [ V- (GND) ] ──────────────────┐ │
                 │ │                                                 │ │
                 │ │ (Dây Mass chung bắt buộc)                       │ │
                 │ ▼                                                 │ │
            ┌─────────┐                                              │ │
            │  [GND]  │                                              │ │
 ┌──────────┴─────────┴──────────┐                                   │ │
 │       BO MẠCH ESP32           │                                   │ │
 │                               │                                   │ │
 │   [Cổng USB] ◄───(Cáp USB)────┼───► [ LAPTOP ]                    │ │
 │                               │     - Cấp nguồn 5V nuôi ESP32     │ │
 │                               │     - Nạp firmware (PIO Upload)   │ │
 │                               │     - Serial Monitor (Giả lập L/R)│ │
 │                               │                                   │ │
 │   [GPIO 32] ─────────┐        │                                   │ │
 └──────────┬───────────┼────────┘                                   │ │
            │           │                                            │ │
            │ (13 Chân) │                                            │ │
            │ Tín hiệu  │                                            │ │
            │  HUB75    ▼                                            │ │
            │     ┌───────────┐                                      │ │
            │     │ Nút Bấm   │                                      │ │
            │     │  LA16-A   │ [Chân NO] ◄─── (GPIO 32)             │ │
            │     │           │ [Chân C ] ◄─── (GND ESP32)           │ │
            │     │  (E-Stop) │ [Chân NC] ◄─── (Bỏ trống)            │ │
            │     └───────────┘                                      │ │
            ▼                                                        ▼ ▼
 ┌─────────────────────────────────────────────────────────────────────┐
 │                    TẤM MA TRẬN LED P5 (HUB75 64x32)                 │
 │                                                                     │
 │   - Cổng Nguồn 4-Pin:  [ VCC (+5V) ] ◄── Nguồn tổ ong (V+)          │
 │                        [ GND (0V)  ] ◄── Nguồn tổ ong (V-)          │
 │                                                                     │
 │   - Cổng Tín hiệu 16-Pin HUB75:                                     │
 │     R1, G1, B1, R2, G2, B2, A, B, C, D, CLK, LAT, OE ◄── Từ ESP32  │
 └─────────────────────────────────────────────────────────────────────┘
```

---

## 3. Hướng dẫn Đấu dây Chi tiết

### 3.1. Cấp nguồn Tấm LED P5 (Dòng cao)
- Dây đỏ tấm LED $\rightarrow$ Nối vào cổng **`V+`** (+5V) của nguồn tổ ong.
- Dây đen tấm LED $\rightarrow$ Nối vào cổng **`V-`** (GND) của nguồn tổ ong.
- ⚠️ **Dây Mass chung (Bắt buộc):** Kéo thêm một sợi dây từ cổng **`V-`** của nguồn tổ ong cắm vào chân **`GND`** bất kỳ trên ESP32.

### 3.2. Cáp Tín hiệu HUB75 (16-Pin Ribbon Cable)
Nối từ các chân GPIO của ESP32 sang đầu cắm HUB75 trên tấm P5:

```text
                  Cổng cắm HUB75 Tấm P5 (Nhìn từ sau lưng)
                             ┌───┬───┐
      (GPIO 25)  R1   [ 1]  │ • │ • │  [ 2]   G1   (GPIO 26)
      (GPIO 27)  B1   [ 3]  │ • │ • │  [ 4]   GND  (Nối GND ESP32)
      (GPIO 14)  R2   [ 5]  │ • │ • │  [ 6]   G2   (GPIO 12)
      (GPIO 13)  B2   [ 7]  │ • │ • │  [ 8]   GND  (Nối GND ESP32)
      (GPIO 23)   A   [ 9]  │ • │ • │  [10]    B   (GPIO 19)
      (GPIO  5)   C   [11]  │ • │ • │  [12]    D   (GPIO 17)
      (GPIO 16) CLK   [13]  │ • │ • │  [14]  LAT   (GPIO 4)
      (GPIO 15)  OE   [15]  │ • │ • │  [16]   GND  (Nối GND ESP32)
                             └───┴───┘
```

### 3.3. Nút Dừng Khẩn Cấp LA16-A
- Chân **`C`** (Common): Nối vào chân **`GND`** của ESP32.
- Chân **`NO`** (Normally Open): Nối vào chân **`GPIO 32`** của ESP32.
- Chân **`NC`**: Để trống.
- *Nguyên lý:* Mặc định chân GPIO 32 được kéo lên mức HIGH (3.3V). Khi bấm nút, tiếp điểm đóng kéo GPIO 32 về mức LOW (0V) $\rightarrow$ ESP32 ngắt hiển thị LED ngay tức thì và gửi sự kiện E-Stop.

### 3.4. Kết nối Máy tính & Giả lập Tín hiệu Ô tô
- Cắm cáp USB nối Laptop với ESP32.
- Mở **Serial Monitor** (Baud rate: **115200**).
- Gõ các phím sau vào Serial Monitor để giả lập tín hiệu xe ô tô:
  - Phím **`L`** hoặc **`l`**: Giả lập Xi-nhan Trái (Mũi tên đuổi màu vàng sang trái `<<<`).
  - Phím **`R`** hoặc **`r`**: Giả lập Xi-nhan Phải (Mũi tên đuổi màu vàng sang phải `>>>`).
  - Phím **`H`** hoặc **`h`**: Giả lập Đèn cảnh báo khẩn cấp Hazard (Mũi tên đôi bung sang 2 bên `<<< >>>`).
  - Phím **`S`**, **`s`** hoặc **`0`**: Tắt xi-nhan (Trả về hiệu ứng LED bình thường).

---

## 4. Các Lưu ý An toàn Quan trọng trên Bàn Thử Nghiệm

1. **Không cấp nguồn ngược cực:** Nguồn tổ ong 5V 20A có dòng ngắn mạch rất lớn. Tuyệt đối kiểm tra kỹ cực tính `V+` (Đỏ) và `V-` (Đen) trước khi bật nguồn 220V.
2. **Không nối chân 5V của nguồn tổ ong vào chân 5V/VIN của ESP32 khi đang cắm USB:** 
   - Vì ESP32 đang được cấp nguồn trực tiếp từ cổng USB laptop (5.0V), nếu bạn nối thêm đường 5V từ nguồn tổ ong vào chân VIN của ESP32 mà điện áp hai bên chênh lệch nhau, dòng điện có thể xông ngược vào cổng USB làm hỏng cổng laptop.
   - **Quy tắc an toàn:** Nguồn tổ ong **chỉ cấp nguồn 5V cho tấm LED P5** và **chỉ nối chung dây GND** với ESP32.
