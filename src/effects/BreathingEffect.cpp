#include "BreathingEffect.h"
#include "drivers/LedDriver.h"
#include <math.h>

void BreathingEffect::init() {
    phase = 0.0f;
}

void BreathingEffect::update(const DeviceSettings& settings) {
    // Speed maps roughly to frequency
    float speedFactor = 0.02f + (static_cast<float>(settings.speed) / 100.0f) * 0.08f;
    phase += speedFactor;
    if (phase >= 6.28318f) phase -= 6.28318f;

    // Sinus wave scaled to 0.1 - 1.0
    float breath = (sinf(phase) + 1.0f) / 2.0f;
    float factor = 0.15f + 0.85f * breath;

    uint8_t r = static_cast<uint8_t>(settings.primaryColor.r * factor);
    uint8_t g = static_cast<uint8_t>(settings.primaryColor.g * factor);
    uint8_t b = static_cast<uint8_t>(settings.primaryColor.b * factor);

    LedDriver::getInstance().fill(r, g, b);
}
