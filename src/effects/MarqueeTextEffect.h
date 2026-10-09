#pragma once

#include "IEffect.h"
#include "types.h"
#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

class MarqueeTextEffect : public IEffect {
public:
    MarqueeTextEffect();
    ~MarqueeTextEffect();

    void init() override;
    void update(const DeviceSettings& settings) override;
    const char* getName() const override { return "Marquee Text"; }
    EffectMode getMode() const override { return EffectMode::MARQUEE_TEXT; }

    void configure(const String& newText, const RGBColor& col, uint8_t spd, uint8_t sz, int16_t y, bool rainbow, const String& scMode);
    void setText(const String& newText);
    void setColor(const RGBColor& col);
    void setSpeed(uint8_t spd);
    void setTextSize(uint8_t sz);
    void setYPos(int16_t y);
    void setRainbow(bool enable);
    void setScrollMode(const String& scMode);

    static String normalizeText(const String& input);

private:
    String rawText;
    String displayText;
    RGBColor textColor;
    uint8_t speed;          // 1 - 100
    uint8_t textSize;       // 1 or 2
    int16_t yPos;           // -1 for auto-center
    bool isRainbow;
    String scrollMode;      // "scroll_left", "static", "blink"

    int16_t cursorX;
    int16_t textPixelWidth;
    unsigned long lastStepTick;
    uint8_t rainbowHue;
    SemaphoreHandle_t mutex;

    void recalculateBounds();
};
