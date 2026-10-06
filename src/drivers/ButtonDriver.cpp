#include "ButtonDriver.h"
#include "config.h"
#include "utils/Logger.h"

#define TAG "BUTTON"
#define DEBOUNCE_DELAY_MS 50
#define LONG_PRESS_MS     1200

ButtonDriver& ButtonDriver::getInstance() {
    static ButtonDriver instance;
    return instance;
}

ButtonDriver::ButtonDriver()
    : clickCb(nullptr), longPressCb(nullptr),
      lastState(HIGH), pressStartTime(0), isLongPressed(false) {}

void ButtonDriver::init() {
    pinMode(PIN_BUTTON_MODE, INPUT_PULLUP);
    LOG_I(TAG, "Button Driver initialized on GPIO %d", PIN_BUTTON_MODE);
}

void ButtonDriver::onClick(ClickCallback cb) {
    clickCb = cb;
}

void ButtonDriver::onLongPress(LongPressCallback cb) {
    longPressCb = cb;
}

void ButtonDriver::update() {
    bool currentState = digitalRead(PIN_BUTTON_MODE);

    // Button pressed (Active LOW)
    if (currentState == LOW && lastState == HIGH) {
        pressStartTime = millis();
        isLongPressed = false;
    }
    // Button held down
    else if (currentState == LOW && lastState == LOW) {
        if (!isLongPressed && (millis() - pressStartTime >= LONG_PRESS_MS)) {
            isLongPressed = true;
            if (longPressCb) {
                longPressCb();
            }
        }
    }
    // Button released
    else if (currentState == HIGH && lastState == LOW) {
        unsigned long duration = millis() - pressStartTime;
        if (duration >= DEBOUNCE_DELAY_MS && !isLongPressed) {
            if (clickCb) {
                clickCb();
            }
        }
    }

    lastState = currentState;
}
