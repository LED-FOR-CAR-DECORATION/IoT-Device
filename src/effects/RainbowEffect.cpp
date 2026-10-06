#include "RainbowEffect.h"
#include "drivers/LedDriver.h"
#include "utils/ColorUtils.h"

void RainbowEffect::init() {
    hueOffset = 0;
}

void RainbowEffect::update(const DeviceSettings& settings) {
    LedDriver& led = LedDriver::getInstance();
    uint16_t numLeds = led.getNumLeds();

    uint8_t deltaHue = 255 / numLeds;
    for (uint16_t i = 0; i < numLeds; i++) {
        uint8_t pixelHue = (hueOffset + (i * deltaHue)) & 0xFF;
        led.setPixel(i, ColorUtils::hsvToRgb(pixelHue, 240, 255));
    }

    uint8_t step = map(settings.speed, 1, 100, 1, 6);
    hueOffset = (hueOffset + step) & 0xFF;
}
