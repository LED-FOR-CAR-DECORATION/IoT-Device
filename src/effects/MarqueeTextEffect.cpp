#include "MarqueeTextEffect.h"
#include "drivers/LedDriver.h"
#include "utils/ColorUtils.h"
#include "utils/Logger.h"

#define TAG "MARQUEE"

MarqueeTextEffect::MarqueeTextEffect()
    : rawText("XIN CHAO!"), displayText("XIN CHAO!"), textColor(255, 215, 0),
      speed(60), textSize(1), yPos(-1), isRainbow(false), scrollMode("scroll_left"),
      cursorX(64), textPixelWidth(54), lastStepTick(0), rainbowHue(0) {
    mutex = xSemaphoreCreateMutex();
}

MarqueeTextEffect::~MarqueeTextEffect() {
    if (mutex) {
        vSemaphoreDelete(mutex);
    }
}

void MarqueeTextEffect::init() {
    if (xSemaphoreTake(mutex, pdMS_TO_TICKS(50)) == pdTRUE) {
        lastStepTick = millis();
        rainbowHue = 0;
        recalculateBounds();
        xSemaphoreGive(mutex);
    }
}

void MarqueeTextEffect::recalculateBounds() {
    int16_t charWidth = 6 * (textSize > 0 ? textSize : 1);
    textPixelWidth = (int16_t)displayText.length() * charWidth;

    if (scrollMode == "scroll_left") {
        cursorX = 64; // Start from right edge
    } else {
        cursorX = (64 - textPixelWidth) / 2; // Center statically
    }
}

String MarqueeTextEffect::normalizeText(const String& input) {
    String output = "";
    output.reserve(input.length());
    size_t len = input.length();
    size_t i = 0;

    while (i < len) {
        uint8_t c = (uint8_t)input[i];

        if (c < 0x80) {
            // Standard ASCII
            if (c >= 32 || c == '\n') {
                output += (char)c;
            }
            i++;
        } else if (c == 0xC3 && i + 1 < len) {
            // 2-byte Latin-1 supplement
            uint8_t b2 = (uint8_t)input[i + 1];
            if ((b2 >= 0x80 && b2 <= 0x85)) output += 'A';
            else if ((b2 >= 0xA0 && b2 <= 0xA5)) output += 'a';
            else if ((b2 >= 0x88 && b2 <= 0x8B)) output += 'E';
            else if ((b2 >= 0xA8 && b2 <= 0xAB)) output += 'e';
            else if ((b2 >= 0x8C && b2 <= 0x8F)) output += 'I';
            else if ((b2 >= 0xAC && b2 <= 0xAF)) output += 'i';
            else if ((b2 >= 0x92 && b2 <= 0x96)) output += 'O';
            else if ((b2 >= 0xB2 && b2 <= 0xB6)) output += 'o';
            else if ((b2 >= 0x99 && b2 <= 0x9C)) output += 'U';
            else if ((b2 >= 0xB9 && b2 <= 0xBC)) output += 'u';
            else if (b2 == 0x9D) output += 'Y';
            else if (b2 == 0xBD) output += 'y';
            else output += ' ';
            i += 2;
        } else if (c == 0xC4 && i + 1 < len) {
            uint8_t b2 = (uint8_t)input[i + 1];
            if (b2 == 0x82) output += 'A';
            else if (b2 == 0x83) output += 'a';
            else if (b2 == 0x90) output += 'D';
            else if (b2 == 0x91) output += 'd';
            else output += ' ';
            i += 2;
        } else if (c == 0xC5 && i + 1 < len) {
            uint8_t b2 = (uint8_t)input[i + 1];
            if (b2 == 0xA8) output += 'U';
            else if (b2 == 0xA9) output += 'u';
            else if (b2 == 0xA0) output += 'O';
            else if (b2 == 0xA1) output += 'o';
            else output += ' ';
            i += 2;
        } else if (c == 0xE1 && i + 2 < len) {
            // 3-byte Vietnamese precomposed characters (U+1EA0 - U+1EF9)
            uint8_t b2 = (uint8_t)input[i + 1];
            uint8_t b3 = (uint8_t)input[i + 2];
            if (b2 == 0xBA) {
                if (b3 >= 0xA0 && b3 <= 0xB7) {
                    output += ((b3 % 2 == 0) ? 'A' : 'a');
                } else if (b3 >= 0xB8 && b3 <= 0xBF) {
                    output += ((b3 % 2 == 0) ? 'E' : 'e');
                } else {
                    output += ' ';
                }
            } else if (b2 == 0xBB) {
                if (b3 >= 0x80 && b3 <= 0x87) {
                    output += ((b3 % 2 == 0) ? 'E' : 'e');
                } else if (b3 >= 0x88 && b3 <= 0x8B) {
                    output += ((b3 % 2 == 0) ? 'I' : 'i');
                } else if (b3 >= 0x8C && b3 <= 0xA3) {
                    output += ((b3 % 2 == 0) ? 'O' : 'o');
                } else if (b3 >= 0xA4 && b3 <= 0xB1) {
                    output += ((b3 % 2 == 0) ? 'U' : 'u');
                } else if (b3 >= 0xB2 && b3 <= 0xB9) {
                    output += ((b3 % 2 == 0) ? 'Y' : 'y');
                } else {
                    output += ' ';
                }
            } else {
                output += ' ';
            }
            i += 3;
        } else {
            i++;
        }
    }
    return output;
}

void MarqueeTextEffect::configure(const String& newText, const RGBColor& col, uint8_t spd, uint8_t sz, int16_t y, bool rainbow, const String& scMode) {
    if (xSemaphoreTake(mutex, pdMS_TO_TICKS(50)) == pdTRUE) {
        rawText = newText;
        displayText = normalizeText(newText);
        textColor = col;
        speed = spd > 0 ? (spd <= 100 ? spd : 100) : 50;
        textSize = (sz == 2) ? 2 : 1;
        yPos = y;
        isRainbow = rainbow;
        scrollMode = scMode;
        lastStepTick = millis();
        recalculateBounds();
        xSemaphoreGive(mutex);
        LOG_I(TAG, "Configured marquee: text='%s' (display='%s'), sz=%d, spd=%d, mode=%s",
              rawText.c_str(), displayText.c_str(), textSize, speed, scrollMode.c_str());
    }
}

void MarqueeTextEffect::setText(const String& newText) {
    if (xSemaphoreTake(mutex, pdMS_TO_TICKS(50)) == pdTRUE) {
        rawText = newText;
        displayText = normalizeText(newText);
        recalculateBounds();
        xSemaphoreGive(mutex);
    }
}

void MarqueeTextEffect::setColor(const RGBColor& col) {
    if (xSemaphoreTake(mutex, pdMS_TO_TICKS(50)) == pdTRUE) {
        textColor = col;
        xSemaphoreGive(mutex);
    }
}

void MarqueeTextEffect::setSpeed(uint8_t spd) {
    if (xSemaphoreTake(mutex, pdMS_TO_TICKS(50)) == pdTRUE) {
        speed = spd > 0 ? (spd <= 100 ? spd : 100) : 50;
        xSemaphoreGive(mutex);
    }
}

void MarqueeTextEffect::setTextSize(uint8_t sz) {
    if (xSemaphoreTake(mutex, pdMS_TO_TICKS(50)) == pdTRUE) {
        textSize = (sz == 2) ? 2 : 1;
        recalculateBounds();
        xSemaphoreGive(mutex);
    }
}

void MarqueeTextEffect::setYPos(int16_t y) {
    if (xSemaphoreTake(mutex, pdMS_TO_TICKS(50)) == pdTRUE) {
        yPos = y;
        xSemaphoreGive(mutex);
    }
}

void MarqueeTextEffect::setRainbow(bool enable) {
    if (xSemaphoreTake(mutex, pdMS_TO_TICKS(50)) == pdTRUE) {
        isRainbow = enable;
        xSemaphoreGive(mutex);
    }
}

void MarqueeTextEffect::setScrollMode(const String& scMode) {
    if (xSemaphoreTake(mutex, pdMS_TO_TICKS(50)) == pdTRUE) {
        scrollMode = scMode;
        recalculateBounds();
        xSemaphoreGive(mutex);
    }
}

void MarqueeTextEffect::update(const DeviceSettings& settings) {
    LedDriver& led = LedDriver::getInstance();
    MatrixPanel_I2S_DMA* display = led.getDisplay();
    if (!display) return;

    if (xSemaphoreTake(mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
        unsigned long now = millis();
        // Speed 1 -> 85ms, Speed 100 -> 15ms
        uint16_t stepDelay = 85 - (speed * 70 / 100);
        if (stepDelay < 12) stepDelay = 12;

        if (scrollMode == "scroll_left") {
            if (now - lastStepTick >= stepDelay) {
                lastStepTick = now;
                cursorX--;
                if (cursorX < -textPixelWidth) {
                    cursorX = led.getWidth();
                }
            }
        } else {
            cursorX = (led.getWidth() - textPixelWidth) / 2;
        }

        // Calculate Y position
        int16_t actualY = yPos;
        if (actualY < 0) {
            actualY = (led.getHeight() - (8 * textSize)) / 2;
        }

        // Render frame
        led.clear();
        display->setTextWrap(false);
        display->setTextSize(textSize);

        bool shouldRender = true;
        if (scrollMode == "blink") {
            shouldRender = ((now / 400) % 2 == 0);
        }

        if (shouldRender) {
            int16_t charWidth = 6 * textSize;
            if (isRainbow) {
                for (size_t i = 0; i < displayText.length(); i++) {
                    int16_t cx = cursorX + i * charWidth;
                    if (cx > -charWidth && cx < (int16_t)led.getWidth()) {
                        uint8_t h = (rainbowHue + i * 18) & 0xFF;
                        RGBColor rgb = ColorUtils::hsvToRgb(h, 240, 255);
                        uint16_t col565 = ColorUtils::color565(rgb.r, rgb.g, rgb.b);
                        display->setTextColor(col565);
                        display->setCursor(cx, actualY);
                        display->print(displayText[i]);
                    }
                }
                rainbowHue = (rainbowHue + 2) & 0xFF;
            } else {
                uint16_t col565 = ColorUtils::color565(textColor.r, textColor.g, textColor.b);
                display->setTextColor(col565);
                display->setCursor(cursorX, actualY);
                display->print(displayText);
            }
        }

        xSemaphoreGive(mutex);
    }
}
