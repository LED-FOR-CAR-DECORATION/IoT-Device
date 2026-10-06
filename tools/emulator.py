#!/usr/bin/env python3
"""
Car LED IoT Device Emulator
Giả lập mạch ESP32 LED trên máy tính để đội ngũ Mobile App test tính năng
mà không cần phần cứng thật.

Hỗ trợ:
1. Lắng nghe lệnh MQTT từ Broker (cùng broker với App)
2. Mở cổng TCP socket nội bộ (Port 8888) cho App kết nối trực tiếp
3. Render dải LED 30 pixel ảo có màu sắc sống động ngay trên Terminal
"""

import sys
import json
import time
import math
import socket
import threading

NUM_LEDS = 30

class VirtualLedStrip:
    def __init__(self, count=NUM_LEDS):
        self.count = count
        self.power = True
        self.brightness = 180
        self.mode = 3 # 3: Rainbow Wave
        self.speed = 50
        self.color = [0, 200, 255]
        self.sensitivity = 75
        self.leds = [[0, 0, 0] for _ in range(count)]
        self.tick = 0

    def apply_command(self, cmd: dict):
        if "power" in cmd:
            self.power = bool(cmd["power"])
        if "brightness" in cmd:
            self.brightness = max(0, min(255, int(cmd["brightness"])))
        if "mode" in cmd:
            self.mode = int(cmd["mode"])
        if "speed" in cmd:
            self.speed = max(1, min(100, int(cmd["speed"])))
        if "color" in cmd and isinstance(cmd["color"], dict):
            c = cmd["color"]
            self.color = [c.get("r", 255), c.get("g", 255), c.get("b", 255)]
        if "sensitivity" in cmd:
            self.sensitivity = int(cmd["sensitivity"])

    def update_frame(self):
        if not self.power or self.mode == 0:
            self.leds = [[0, 0, 0] for _ in range(self.count)]
            return

        scale = self.brightness / 255.0

        # Mode 1: Static Color
        if self.mode == 1:
            r = int(self.color[0] * scale)
            g = int(self.color[1] * scale)
            b = int(self.color[2] * scale)
            self.leds = [[r, g, b] for _ in range(self.count)]

        # Mode 2: Breathing
        elif self.mode == 2:
            breath = (math.sin(self.tick * 0.1 * (self.speed / 50.0)) + 1.0) / 2.0
            factor = scale * breath
            r = int(self.color[0] * factor)
            g = int(self.color[1] * factor)
            b = int(self.color[2] * factor)
            self.leds = [[r, g, b] for _ in range(self.count)]

        # Mode 3: Rainbow Wave
        elif self.mode == 3:
            for i in range(self.count):
                hue = (self.tick * (self.speed * 0.1) + (i * 12)) % 360
                r, g, b = self.hsv_to_rgb(hue, 1.0, scale)
                self.leds[i] = [r, g, b]

        # Mode 4: Chase Flow
        elif self.mode == 4:
            head = int((self.tick * (self.speed * 0.05)) % self.count)
            for i in range(self.count):
                dist = (i - head) % self.count
                if dist < 6:
                    dim = (1.0 - dist / 6.0) * scale
                    self.leds[i] = [int(self.color[0] * dim), int(self.color[1] * dim), int(self.color[2] * dim)]
                else:
                    self.leds[i] = [0, 0, 0]

        # Mode 5: Music Reactive (Mô phỏng cột âm thanh)
        elif self.mode == 5:
            amp = (math.sin(self.tick * 0.3) * 0.5 + 0.5) * (self.sensitivity / 100.0)
            lit = int(self.count * amp)
            for i in range(self.count):
                if i < lit:
                    self.leds[i] = [int(255 * scale), int(50 * scale), int(100 * scale)]
                else:
                    self.leds[i] = [10, 10, 10]

        self.tick += 1

    @staticmethod
    def hsv_to_rgb(h, s, v):
        c = v * s
        x = c * (1 - abs((h / 60.0) % 2 - 1))
        m = v - c
        if 0 <= h < 60: rgb = (c, x, 0)
        elif 60 <= h < 120: rgb = (x, c, 0)
        elif 120 <= h < 180: rgb = (0, c, x)
        elif 180 <= h < 240: rgb = (0, x, c)
        elif 240 <= h < 300: rgb = (x, 0, c)
        else: rgb = (c, 0, x)
        return int((rgb[0] + m) * 255), int((rgb[1] + m) * 255), int((rgb[2] + m) * 255)

    def render_console(self):
        strip_str = "".join([f"\033[38;2;{r};{g};{b}m█\033[0m" for r, g, b in self.leds])
        mode_names = ["OFF", "STATIC", "BREATHE", "RAINBOW", "CHASE", "MUSIC"]
        m_name = mode_names[self.mode] if self.mode < len(mode_names) else "CUSTOM"
        status = f"PWR: {'ON' if self.power else 'OFF'} | BRT: {self.brightness:3d} | MODE: {m_name:7s} | SPD: {self.speed:3d}"
        sys.stdout.write(f"\r{strip_str}  [{status}]")
        sys.stdout.flush()

def start_tcp_listener(strip: VirtualLedStrip, port=8888):
    server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    server.bind(("0.0.0.0", port))
    server.listen(5)
    print(f"📡 [TCP Emulator] Đang lắng nghe App kết nối tại cổng: {port}")

    while True:
        client, addr = server.accept()
        print(f"\n📱 [App Connected] Điện thoại đã kết nối từ: {addr}")
        def handle_client(c):
                try:
                    data = c.recv(1024)
                    if not data: break
                    # Check 6-byte binary frame: [0xAA, CMD, P1, P2, P3, 0x55]
                    if len(data) == 6 and data[0] == 0xAA and data[5] == 0x55:
                        opcode, p1, p2, p3 = data[1], data[2], data[3], data[4]
                        if opcode == 0x01: strip.power = (p1 != 0)
                        elif opcode == 0x02: strip.brightness = p1
                        elif opcode == 0x03: strip.speed = p1
                        elif opcode == 0x04: strip.mode = p1
                        elif opcode == 0x05: strip.color = [p1, p2, p3]
                        elif opcode == 0x06: strip.sensitivity = p1
                        c.send(b'\xAA\x00\x55')
                        continue

                    text = data.decode("utf-8").strip()
                    try:
                        cmd = json.loads(text)
                        strip.apply_command(cmd)
                        c.send(b'{"status":"ok"}\n')
                    except Exception as err:
                        c.send(f'{{"error":"{err}"}}\n'.encode("utf-8"))
                except:
                    break
            c.close()
        threading.Thread(target=handle_client, args=(client,), daemon=True).start()

def main():
    print("=" * 65)
    print("      CAR LED AMBIENT LIGHTING - IOT HARDWARE EMULATOR")
    print("=" * 65)
    print("Hướng dẫn: Mobile App có thể gửi chuỗi JSON qua Socket Port 8888")
    print("Ví dụ: {\"mode\": 3, \"speed\": 80, \"brightness\": 255}\n")

    strip = VirtualLedStrip()
    threading.Thread(target=start_tcp_listener, args=(strip,), daemon=True).start()

    try:
        while True:
            strip.update_frame()
            strip.render_console()
            time.sleep(0.04) # ~25 FPS
    except KeyboardInterrupt:
        print("\n\nĐã dừng mô phỏng.")

if __name__ == "__main__":
    main()
