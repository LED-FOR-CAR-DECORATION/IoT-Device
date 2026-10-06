#pragma once

#include "types.h"
#include <Preferences.h>

class StateManager {
public:
    static StateManager& getInstance();

    void init();
    DeviceSettings& getSettings();
    void updateSettings(const DeviceSettings& newSettings);
    void saveToNVS();
    void loadFromNVS();

    // Convenience setters
    void setPower(bool on);
    void setBrightness(uint8_t brightness);
    void setMode(EffectMode mode);
    void setSpeed(uint8_t speed);
    void setPrimaryColor(uint8_t r, uint8_t g, uint8_t b);

private:
    StateManager();
    Preferences preferences;
    DeviceSettings currentSettings;
    bool isDirty;
    unsigned long lastSaveTime;
};
