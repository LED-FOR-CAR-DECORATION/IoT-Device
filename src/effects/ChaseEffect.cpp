#include "ChaseEffect.h"
#include "drivers/LedDriver.h"
#include "utils/ColorUtils.h"

void ChaseEffect::init() {
    headPos = 0;
}

void ChaseEffect::update(const DeviceSettings& settings) {
    LedDriver& led = LedDriver::getInstance();
    uint16_t numLeds = led.getNumLeds();
    CRGB* leds = led.getLeds();

    // Fade all LEDs slightly each frame to create a trailing comet effect
    fadeToBlackBy(leds, numLeds, 40);

    // Set head pixel
    led.setPixel(headPos, ColorUtils::toCRGB(settings.primaryColor));

    headPos = (headPos + 1) % numLeds;
}
