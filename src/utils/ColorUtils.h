#pragma once

#include "types.h"
#include <FastLED.h>

class ColorUtils {
public:
    static CRGB toCRGB(const RGBColor& col) {
        return CRGB(col.r, col.g, col.b);
    }

    static RGBColor fromCRGB(const CRGB& col) {
        return RGBColor(col.r, col.g, col.b);
    }

    static CRGB hsvToRgb(uint8_t h, uint8_t s, uint8_t v) {
        CHSV hsv(h, s, v);
        CRGB rgb;
        hsv2rgb_rainbow(hsv, rgb);
        return rgb;
    }
};
