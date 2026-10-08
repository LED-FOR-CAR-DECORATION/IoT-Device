#include "LedDriver.h"
#include "utils/Logger.h"

#define TAG "HUB75"

LedDriver& LedDriver::getInstance() {
    static LedDriver instance;
    return instance;
}

LedDriver::LedDriver() 
    : dmaDisplay(nullptr), currentBrightness(DEFAULT_BRIGHTNESS) {}

void LedDriver::init() {
    HUB75_I2S_CFG::i2s_pins _pins = {
        PIN_HUB75_R1, PIN_HUB75_G1, PIN_HUB75_B1,
        PIN_HUB75_R2, PIN_HUB75_G2, PIN_HUB75_B2,
        PIN_HUB75_A,  PIN_HUB75_B,  PIN_HUB75_C,  PIN_HUB75_D,  PIN_HUB75_E,
        PIN_HUB75_LAT, PIN_HUB75_OE, PIN_HUB75_CLK
    };

    HUB75_I2S_CFG mxconfig(
        MATRIX_WIDTH,   // 64
        MATRIX_HEIGHT,  // 32
        MATRIX_CHAIN,   // 1
        _pins
    );

    // Optimize DMA double-buffering & color depth for car display
    mxconfig.double_buff = false;
    mxconfig.clkphase = false;
    mxconfig.driver = HUB75_I2S_CFG::SHIFTREG;

    dmaDisplay = new MatrixPanel_I2S_DMA(mxconfig);
    if (!dmaDisplay->begin()) {
        LOG_E(TAG, "****** HUB75 DMA Matrix Initialization FAILED! ******");
        return;
    }

    dmaDisplay->setBrightness8(currentBrightness);
    dmaDisplay->clearScreen();

    LOG_I(TAG, "HUB75 DMA Matrix %dx%d Initialized successfully!", MATRIX_WIDTH, MATRIX_HEIGHT);
}

void LedDriver::setBrightness(uint8_t brightness) {
    currentBrightness = brightness;
    if (dmaDisplay) {
        dmaDisplay->setBrightness8(brightness);
    }
}

void LedDriver::clear() {
    if (dmaDisplay) {
        dmaDisplay->clearScreen();
    }
}

void LedDriver::show() {
    // DMA continuously pushes frames automatically via I2S
}

void LedDriver::drawPixel(int16_t x, int16_t y, uint8_t r, uint8_t g, uint8_t b) {
    if (dmaDisplay && x >= 0 && x < MATRIX_WIDTH && y >= 0 && y < MATRIX_HEIGHT) {
        dmaDisplay->drawPixelRGB888(x, y, r, g, b);
    }
}

void LedDriver::drawPixel(int16_t x, int16_t y, uint16_t color565) {
    if (dmaDisplay && x >= 0 && x < MATRIX_WIDTH && y >= 0 && y < MATRIX_HEIGHT) {
        dmaDisplay->drawPixel(x, y, color565);
    }
}

void LedDriver::fill(uint8_t r, uint8_t g, uint8_t b) {
    if (dmaDisplay) {
        dmaDisplay->fillScreenRGB888(r, g, b);
    }
}

void LedDriver::fill(uint16_t color565) {
    if (dmaDisplay) {
        dmaDisplay->fillScreen(color565);
    }
}

void LedDriver::drawText(int16_t x, int16_t y, const String& text, uint16_t color565, uint8_t textSize) {
    if (!dmaDisplay) return;
    dmaDisplay->setTextSize(textSize);
    dmaDisplay->setTextColor(color565);
    dmaDisplay->setCursor(x, y);
    dmaDisplay->print(text);
}

void LedDriver::drawBitmap(int16_t x, int16_t y, const uint16_t* bitmap, int16_t w, int16_t h) {
    if (!dmaDisplay || !bitmap) return;
    dmaDisplay->drawRGBBitmap(x, y, bitmap, w, h);
}

uint16_t LedDriver::color565(uint8_t r, uint8_t g, uint8_t b) {
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}
