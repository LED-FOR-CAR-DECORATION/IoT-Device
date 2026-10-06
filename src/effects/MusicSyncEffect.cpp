#include "MusicSyncEffect.h"
#include "drivers/LedDriver.h"
#include "drivers/AudioSensor.h"
#include "utils/ColorUtils.h"

void MusicSyncEffect::init() {
    peakHold = 0;
    lastDecay = 0;
}

void MusicSyncEffect::update(const DeviceSettings& settings) {
    LedDriver& led = LedDriver::getInstance();
    uint16_t numLeds = led.getNumLeds();
    uint16_t halfLeds = numLeds / 2;

    uint8_t audioLevel = AudioSensor::getInstance().getNormalizedLevel(settings.micSensitivity);
    uint16_t litLeds = map(audioLevel, 0, 255, 0, halfLeds);

    led.clear();

    // Symmetrical VU-meter spreading from center to edges
    for (uint16_t i = 0; i < litLeds; i++) {
        uint8_t hue = map(i, 0, halfLeds, 160, 0); // Blue -> Green -> Red
        CRGB color = ColorUtils::hsvToRgb(hue, 255, 255);

        led.setPixel(halfLeds - 1 - i, color);
        led.setPixel(halfLeds + i, color);
    }
}
