#include "StaticColorEffect.h"
#include "drivers/LedDriver.h"

void StaticColorEffect::init() {}

void StaticColorEffect::update(const DeviceSettings& settings) {
    LedDriver& led = LedDriver::getInstance();
    led.fill(settings.primaryColor.r, settings.primaryColor.g, settings.primaryColor.b);
}
