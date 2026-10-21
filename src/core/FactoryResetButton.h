#pragma once
#include <stdint.h>

// Active-low BOOT input is sampled by the main loop, never from an ISR.
class FactoryResetButton {
public:
    bool update(bool pressed, uint32_t now) {
        // A held button across reboot must be released before another reset.
        if (!pressed) armed = true;
        if (pressed != rawPressed) { rawPressed = pressed; changedAt = now; }
        if (pressed != stablePressed && uint32_t(now - changedAt) >= 30) {
            stablePressed = pressed;
            if (pressed) { pressedAt = now; fired = false; }
        }
        if (armed && pressed && stablePressed && !fired && uint32_t(now - pressedAt) >= 5000) {
            fired = true;
            return true;
        }
        return false;
    }
private:
    bool rawPressed = false, stablePressed = false, fired = false, armed = false;
    uint32_t changedAt = 0, pressedAt = 0;
};
