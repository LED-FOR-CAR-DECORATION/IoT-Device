#include "ChaseEffect.h"
#include "drivers/LedDriver.h"
#include "utils/ColorUtils.h"

void ChaseEffect::init() {
    headPos = 0;
}

void ChaseEffect::update(const DeviceSettings& settings) {
    LedDriver& led = LedDriver::getInstance();
    uint16_t width = led.getWidth();
    uint16_t height = led.getHeight();

    led.clear();

    // Draw animated sweeping vertical neon beam across 64 columns
    for (int16_t x = 0; x < width; x++) {
        int16_t dist = abs(x - (int16_t)headPos);
        if (dist > width / 2) dist = width - dist;

        if (dist <= 6) {
            float intensity = 1.0f - (float)dist / 6.0f;
            uint8_t r = (uint8_t)(settings.primaryColor.r * intensity);
            uint8_t g = (uint8_t)(settings.primaryColor.g * intensity);
            uint8_t b = (uint8_t)(settings.primaryColor.b * intensity);

            for (int16_t y = 0; y < height; y++) {
                led.drawPixel(x, y, r, g, b);
            }
        }
    }

    headPos = (headPos + 1) % width;
}
