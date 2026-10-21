#include "FactoryReset.h"
#include <Arduino.h>
#include <Preferences.h>
#include <SPIFFS.h>
#include "utils/Logger.h"

namespace FactoryReset {
bool completePending() {
    Preferences reset;
    if (!reset.begin("factory_reset", false)) return false;
    if (!reset.getBool("pending", false)) { reset.end(); return true; }
    LOG_W("RESET", "Restoring factory defaults...");
    bool success = true;
    for (const char* name : {"ble_identity", "car_led", "wifi_cfg"}) {
        Preferences settings;
        if (!settings.begin(name, false)) { success = false; continue; }
        if (!settings.clear()) success = false;
        settings.end();
    }
    // SPIFFS contains user projections. StorageManager will recreate the blue car.
    if (!SPIFFS.format()) success = false;
    if (success) success = reset.remove("pending");
    reset.end();
    LOG_W("RESET", "%s", success ? "Factory defaults restored" : "Reset incomplete; retrying before startup");
    return success;
}

void request() {
    Preferences reset;
    if (!reset.begin("factory_reset", false)) {
        LOG_E("RESET", "Cannot save factory reset request");
        return;
    }
    const bool saved = reset.putBool("pending", true) == sizeof(bool);
    reset.end();
    if (!saved) { LOG_E("RESET", "Cannot save factory reset request"); return; }
    LOG_W("RESET", "BOOT held for 5 seconds. Restarting to restore factory defaults.");
    Serial.flush();
    ESP.restart();
}
}
