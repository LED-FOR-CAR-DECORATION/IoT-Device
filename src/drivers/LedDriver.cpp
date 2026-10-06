#include "LedDriver.h"
#include "utils/Logger.h"

#define TAG "LED"

LedDriver& LedDriver::getInstance() {
    static LedDriver instance;
    return instance;
}

LedDriver::LedDriver() : currentBrightness(DEFAULT_BRIGHTNESS) {}

void LedDriver::init() {
    FastLED.addLeds<WS2812B, PIN_LED_DATA_MAIN, LED_COLOR_ORDER>(leds, NUM_LEDS_MAIN)
           .setCorrection(TypicalLEDStrip);

    // FastLED current / power limiter for automotive safety
    FastLED.setMaxPowerInVoltsAndMilliamps(5, MAX_CURRENT_MA);
    FastLED.setBrightness(currentBrightness);
    FastLED.clear(true);

    LOG_I(TAG, "LED Driver initialized: %d LEDs on GPIO %d, Max %dmA", 
          NUM_LEDS_MAIN, PIN_LED_DATA_MAIN, MAX_CURRENT_MA);
}

void LedDriver::setBrightness(uint8_t brightness) {
    currentBrightness = brightness;
    FastLED.setBrightness(brightness);
}

void LedDriver::clear() {
    FastLED.clear();
}

void LedDriver::show() {
    FastLED.show();
}

void LedDriver::setPixel(uint16_t index, const CRGB& color) {
    if (index < NUM_LEDS_MAIN) {
        leds[index] = color;
    }
}

void LedDriver::setPixel(uint16_t index, uint8_t r, uint8_t g, uint8_t b) {
    if (index < NUM_LEDS_MAIN) {
        leds[index] = CRGB(r, g, b);
    }
}

void LedDriver::fill(const CRGB& color) {
    fill_solid(leds, NUM_LEDS_MAIN, color);
}

void LedDriver::fill(uint8_t r, uint8_t g, uint8_t b) {
    fill_solid(leds, NUM_LEDS_MAIN, CRGB(r, g, b));
}

CRGB* LedDriver::getLeds() {
    return leds;
}

uint16_t LedDriver::getNumLeds() const {
    return NUM_LEDS_MAIN;
}
