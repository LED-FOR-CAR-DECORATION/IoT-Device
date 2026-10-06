#pragma once

#include <FastLED.h>
#include "config.h"
#include "types.h"

class LedDriver {
public:
    static LedDriver& getInstance();

    void init();
    void setBrightness(uint8_t brightness);
    void clear();
    void show();

    void setPixel(uint16_t index, const CRGB& color);
    void setPixel(uint16_t index, uint8_t r, uint8_t g, uint8_t b);
    void fill(const CRGB& color);
    void fill(uint8_t r, uint8_t g, uint8_t b);

    CRGB* getLeds();
    uint16_t getNumLeds() const;

private:
    LedDriver();
    CRGB leds[NUM_LEDS_MAIN];
    uint8_t currentBrightness;
};
