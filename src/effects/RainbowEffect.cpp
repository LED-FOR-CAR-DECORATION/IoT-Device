#include "RainbowEffect.h"
#include "drivers/LedDriver.h"
#include "utils/ColorUtils.h"

void RainbowEffect::init() {
    hueOffset = 0;
}

void RainbowEffect::update(const DeviceSettings& settings) {
    LedDriver& led = LedDriver::getInstance();
    uint16_t width = led.getWidth();
    uint16_t height = led.getHeight();

    for (int16_t y = 0; y < height; y++) {
        for (int16_t x = 0; x < width; x++) {
            uint8_t pixelHue = (hueOffset + (x * 4) + (y * 2)) & 0xFF;
            RGBColor rgb = ColorUtils::hsvToRgb(pixelHue, 240, 255);
            led.drawPixel(x, y, rgb.r, rgb.g, rgb.b);
        }
    }

    uint8_t step = map(settings.speed, 1, 100, 1, 8);
    hueOffset = (hueOffset + step) & 0xFF;
}
