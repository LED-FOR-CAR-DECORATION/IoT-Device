#pragma once

#include <Arduino.h>
#include "types.h"

class ColorUtils {
public:
    static RGBColor hsvToRgb(uint8_t h, uint8_t s, uint8_t v) {
        if (s == 0) {
            return RGBColor(v, v, v);
        }

        uint8_t region = h / 43;
        uint8_t remainder = (h - (region * 43)) * 6;

        uint8_t p = (v * (255 - s)) >> 8;
        uint8_t q = (v * (255 - ((s * remainder) >> 8))) >> 8;
        uint8_t t = (v * (255 - ((s * (255 - remainder)) >> 8))) >> 8;

        switch (region) {
            case 0:  return RGBColor(v, t, p);
            case 1:  return RGBColor(q, v, p);
            case 2:  return RGBColor(p, v, t);
            case 3:  return RGBColor(p, q, v);
            case 4:  return RGBColor(t, p, v);
            default: return RGBColor(v, p, q);
        }
    }

    static uint16_t color565(uint8_t r, uint8_t g, uint8_t b) {
        return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
    }
};
