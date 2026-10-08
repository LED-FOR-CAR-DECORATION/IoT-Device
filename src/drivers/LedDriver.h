#pragma once

#include <Arduino.h>
#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>
#include "config.h"
#include "pin_definitions.h"
#include "types.h"

#ifndef PIN_HUB75_E
#define PIN_HUB75_E -1
#endif

class LedDriver {
public:
    static LedDriver& getInstance();

    void init();
    void setBrightness(uint8_t brightness);
    void clear();
    void show();

    // Matrix drawing primitives
    void drawPixel(int16_t x, int16_t y, uint8_t r, uint8_t g, uint8_t b);
    void drawPixel(int16_t x, int16_t y, uint16_t color565);
    void fill(uint8_t r, uint8_t g, uint8_t b);
    void fill(uint16_t color565);
    
    // Text and Graphic helpers
    void drawText(int16_t x, int16_t y, const String& text, uint16_t color565, uint8_t textSize = 1);
    void drawBitmap(int16_t x, int16_t y, const uint16_t* bitmap, int16_t w, int16_t h);

    // Helpers
    static uint16_t color565(uint8_t r, uint8_t g, uint8_t b);
    uint16_t getWidth() const { return MATRIX_WIDTH; }
    uint16_t getHeight() const { return MATRIX_HEIGHT; }
    MatrixPanel_I2S_DMA* getDisplay() { return dmaDisplay; }

private:
    LedDriver();
    MatrixPanel_I2S_DMA* dmaDisplay;
    uint8_t currentBrightness;
};
