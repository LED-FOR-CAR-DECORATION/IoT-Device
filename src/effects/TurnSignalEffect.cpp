#include "TurnSignalEffect.h"
#include "drivers/LedDriver.h"

// Automotive Amber Yellow (R: 255, G: 140, B: 0)
static const uint16_t COLOR_AMBER = LedDriver::color565(255, 140, 0);
static const uint16_t COLOR_HAZARD = LedDriver::color565(255, 120, 0);

static const unsigned long CYCLE_DURATION_MS = 650;

TurnSignalEffect::TurnSignalEffect()
    : currentState(TurnSignalState::IDLE), cycleStartTime(0) {}

void TurnSignalEffect::init() {
    cycleStartTime = millis();
}

void TurnSignalEffect::setSignalState(TurnSignalState state) {
    if (currentState != state) {
        currentState = state;
        cycleStartTime = millis(); // Reset cycle on state change
    }
}

void TurnSignalEffect::update(const DeviceSettings& settings) {
    LedDriver& led = LedDriver::getInstance();
    led.clear();

    if (currentState == TurnSignalState::IDLE) {
        return;
    }

    unsigned long elapsed = (millis() - cycleStartTime) % CYCLE_DURATION_MS;

    switch (currentState) {
        case TurnSignalState::RIGHT:
            renderRightTurn(elapsed);
            break;
        case TurnSignalState::LEFT:
            renderLeftTurn(elapsed);
            break;
        case TurnSignalState::HAZARD:
            renderHazard(elapsed);
            break;
        default:
            break;
    }
}

void TurnSignalEffect::renderRightTurn(unsigned long elapsedMs) {
    // 4 Sequential Chevrons pointing Right across 64 pixels
    static const int16_t tips[4] = { 14, 28, 42, 56 };

    // Phased sequential animation (Audi / Lexus style)
    if (elapsedMs < 90) {
        drawChevron(tips[0], true, COLOR_AMBER);
    } else if (elapsedMs < 180) {
        drawChevron(tips[0], true, COLOR_AMBER);
        drawChevron(tips[1], true, COLOR_AMBER);
    } else if (elapsedMs < 270) {
        drawChevron(tips[0], true, COLOR_AMBER);
        drawChevron(tips[1], true, COLOR_AMBER);
        drawChevron(tips[2], true, COLOR_AMBER);
    } else if (elapsedMs < 450) {
        // All arrows solid ON
        drawChevron(tips[0], true, COLOR_AMBER);
        drawChevron(tips[1], true, COLOR_AMBER);
        drawChevron(tips[2], true, COLOR_AMBER);
        drawChevron(tips[3], true, COLOR_AMBER);
    }
    // 450ms - 650ms: Dark pause phase (Blinker off)
}

void TurnSignalEffect::renderLeftTurn(unsigned long elapsedMs) {
    // 4 Sequential Chevrons pointing Left across 64 pixels (flowing right-to-left)
    static const int16_t tips[4] = { 49, 35, 21, 7 };

    if (elapsedMs < 90) {
        drawChevron(tips[0], false, COLOR_AMBER);
    } else if (elapsedMs < 180) {
        drawChevron(tips[0], false, COLOR_AMBER);
        drawChevron(tips[1], false, COLOR_AMBER);
    } else if (elapsedMs < 270) {
        drawChevron(tips[0], false, COLOR_AMBER);
        drawChevron(tips[1], false, COLOR_AMBER);
        drawChevron(tips[2], false, COLOR_AMBER);
    } else if (elapsedMs < 450) {
        // All arrows solid ON
        drawChevron(tips[0], false, COLOR_AMBER);
        drawChevron(tips[1], false, COLOR_AMBER);
        drawChevron(tips[2], false, COLOR_AMBER);
        drawChevron(tips[3], false, COLOR_AMBER);
    }
    // 450ms - 650ms: Dark pause phase (Blinker off)
}

void TurnSignalEffect::renderHazard(unsigned long elapsedMs) {
    // Dual outward-flowing arrows: Left half flows left, Right half flows right
    static const int16_t leftTips[2] = { 20, 6 };
    static const int16_t rightTips[2] = { 43, 57 };

    if (elapsedMs < 150) {
        // Inner arrows light up
        drawChevron(leftTips[0], false, COLOR_HAZARD);
        drawChevron(rightTips[0], true, COLOR_HAZARD);
    } else if (elapsedMs < 420) {
        // Both inner & outer arrows light up
        drawChevron(leftTips[0], false, COLOR_HAZARD);
        drawChevron(leftTips[1], false, COLOR_HAZARD);
        drawChevron(rightTips[0], true, COLOR_HAZARD);
        drawChevron(rightTips[1], true, COLOR_HAZARD);
    }
    // 420ms - 650ms: Dark pause phase
}

void TurnSignalEffect::drawChevron(int16_t xTip, bool pointRight, uint16_t color, uint8_t thickness) {
    LedDriver& led = LedDriver::getInstance();
    const int16_t yCenter = 15;
    const int16_t armHeight = 11; // Reaches from y=4 to y=26

    for (int16_t dy = 0; dy <= armHeight; dy++) {
        int16_t dx = (dy * 7) / armHeight;
        int16_t xBase = pointRight ? (xTip - dx) : (xTip + dx);

        for (uint8_t t = 0; t < thickness; t++) {
            int16_t px = pointRight ? (xBase - t) : (xBase + t);
            led.drawPixel(px, yCenter - dy, color);
            led.drawPixel(px, yCenter + 1 + dy, color);
        }
    }
}
