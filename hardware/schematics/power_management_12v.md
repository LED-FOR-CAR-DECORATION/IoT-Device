# Quản lý Nguồn điện Ô tô 12V (Automotive Power Management)

Hệ thống điện xe ô tô có đặc thù rất khắc nghiệt: điện áp không cố định ở 12V mà dao động từ 11.8V (tắt máy), 14.4V (máy phát đang sạc), và có thể tụt xuống 8V khi đề nổ hoặc vọt lên 40V khi ngắt tải (Load Dump).

---

## 1. Yêu cầu Linh kiện Nguồn

1. **Mạch hạ áp DC-DC (Buck Converter):**
   - Điện áp đầu vào: `DC 9V - 36V` (Dải rộng, bảo vệ chống quá áp).
   - Điện áp đầu ra: `DC 5.0V` ổn định (Sai số < 2%).
   - Dòng tải định mức: **Tối thiểu 8A - 10A (40W - 50W)**.
   - Dạng vỏ: Vỏ nhôm đúc kín tản nhiệt, chống nước chuẩn IP67/IP68 để chịu nhiệt độ cao trong khoang cabin/dưới taplo.

2. **Cầu chì bảo vệ (In-line Fuse):**
   - Loại: Cầu chì ống hoặc cầu chì chân cắm ô tô dạng gài.
   - Thông số: **10A** đặt ngay trên dây dương (+) 12V trước khi vào mạch hạ áp.

---

## 2. Quản lý trạng thái Khóa xe (ACC Power Management)

```text
Chìa khóa xe (ACC) ──► BẬT  ──► ESP32 đọc GPIO 35 = HIGH ──► Phục hồi độ sáng & hiệu ứng cũ từ Flash
Chìa khóa xe (ACC) ──► TẮT  ──► ESP32 đọc GPIO 35 = LOW  ──► Lưu cài đặt vào Flash NVS ──► Fade out tắt LED
```

* **Cơ chế chống cạn ắc quy (Battery Drain Protection):**
  - Khi xe tắt máy qua đêm, hệ thống LED tự động tắt hoàn toàn.
  - ESP32 có thể chuyển sang chế độ ngủ sâu (Deep Sleep) hoặc ngắt nguồn hoàn toàn qua rơ-le để dòng tiêu thụ tĩnh < 5mA, không gây hao bình ắc quy xe.
