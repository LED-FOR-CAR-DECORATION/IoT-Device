#include "OtaManager.h"
#include "config.h"
#include "utils/Logger.h"

#define TAG "OTA"

OtaManager& OtaManager::getInstance() {
    static OtaManager instance;
    return instance;
}

void OtaManager::init() {
    ArduinoOTA.setHostname(DEVICE_NAME);

    ArduinoOTA.onStart([]() {
        LOG_I(TAG, "OTA Update Started...");
    });
    ArduinoOTA.onEnd([]() {
        LOG_I(TAG, "OTA Update Finished successfully! Rebooting...");
    });
    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
        LOG_D(TAG, "OTA Progress: %u%%", (progress / (total / 100)));
    });
    ArduinoOTA.onError([](ota_error_t error) {
        LOG_E(TAG, "OTA Error[%u]", error);
    });

    ArduinoOTA.begin();
    LOG_I(TAG, "OTA Service Ready");
}

void OtaManager::update() {
    ArduinoOTA.handle();
}
