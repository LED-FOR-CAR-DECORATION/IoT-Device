#include "StateManager.h"
#include "utils/Logger.h"

#define TAG "STATE"
#define PREF_NAMESPACE "car_led"

StateManager& StateManager::getInstance() {
    static StateManager instance;
    return instance;
}

StateManager::StateManager() : isDirty(false), lastSaveTime(0), emergencyStopped(false) {}

void StateManager::init() {
    loadFromNVS();
    if (currentSettings.currentMode == EffectMode::RAINBOW_WAVE) {
        setMode(EffectMode::CUSTOM_PRESET);
    }
}

DeviceSettings& StateManager::getSettings() {
    return currentSettings;
}

void StateManager::updateSettings(const DeviceSettings& newSettings) {
    currentSettings = newSettings;
    isDirty = true;
    saveToNVS();
}

void StateManager::setPower(bool on) {
    currentSettings.powerOn = on;
    isDirty = true;
    saveToNVS();
}

void StateManager::setBrightness(uint8_t brightness) {
    currentSettings.brightness = brightness;
    isDirty = true;
    saveToNVS();
}

void StateManager::setMode(EffectMode mode) {
    currentSettings.currentMode = mode;
    isDirty = true;
    saveToNVS();
}

void StateManager::setSpeed(uint8_t speed) {
    currentSettings.speed = speed;
    isDirty = true;
    saveToNVS();
}

void StateManager::setPrimaryColor(uint8_t r, uint8_t g, uint8_t b) {
    currentSettings.primaryColor = RGBColor(r, g, b);
    isDirty = true;
    saveToNVS();
}

void StateManager::loadFromNVS() {
    preferences.begin(PREF_NAMESPACE, true); // Read-only mode
    currentSettings.powerOn = preferences.getBool("pwr", true);
    currentSettings.brightness = preferences.getUChar("bri", 180);
    currentSettings.speed = preferences.getUChar("spd", 50);
    currentSettings.currentMode = static_cast<EffectMode>(preferences.getUChar("mode", static_cast<uint8_t>(EffectMode::CUSTOM_PRESET)));
    
    currentSettings.primaryColor.r = preferences.getUChar("cr_r", 0);
    currentSettings.primaryColor.g = preferences.getUChar("cr_g", 150);
    currentSettings.primaryColor.b = preferences.getUChar("cr_b", 255);
    preferences.end();

    LOG_I(TAG, "Settings loaded from NVS: Mode=%d, Brightness=%d", 
          static_cast<int>(currentSettings.currentMode), currentSettings.brightness);
}

void StateManager::saveToNVS() {
    if (!isDirty) return;

    preferences.begin(PREF_NAMESPACE, false); // Read-write mode
    preferences.putBool("pwr", currentSettings.powerOn);
    preferences.putUChar("bri", currentSettings.brightness);
    preferences.putUChar("spd", currentSettings.speed);
    preferences.putUChar("mode", static_cast<uint8_t>(currentSettings.currentMode));

    preferences.putUChar("cr_r", currentSettings.primaryColor.r);
    preferences.putUChar("cr_g", currentSettings.primaryColor.g);
    preferences.putUChar("cr_b", currentSettings.primaryColor.b);
    preferences.end();

    isDirty = false;
    LOG_I(TAG, "Settings committed to NVS successfully");
}

bool StateManager::isEmergencyStopped() const {
    return emergencyStopped;
}

void StateManager::setEmergencyStopped(bool stopped) {
    emergencyStopped = stopped;
}

String StateManager::getTelemetryJson() const {
    String json = "{";
    json += "\"power\":" + String(currentSettings.powerOn ? "true" : "false") + ",";
    json += "\"brightness\":" + String(currentSettings.brightness) + ",";
    json += "\"speed\":" + String(currentSettings.speed) + ",";
    json += "\"mode\":" + String(static_cast<int>(currentSettings.currentMode)) + ",";
    json += "\"color\":{\"r\":" + String(currentSettings.primaryColor.r) + ",\"g\":" + String(currentSettings.primaryColor.g) + ",\"b\":" + String(currentSettings.primaryColor.b) + "},";
    json += "\"emergency_stop\":" + String(emergencyStopped ? "true" : "false") + ",";
    json += "\"free_heap\":" + String(ESP.getFreeHeap());
    json += "}";
    return json;
}
