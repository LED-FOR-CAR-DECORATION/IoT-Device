#include <Arduino.h>
#include "config.h"
#include "utils/Logger.h"
#include "core/StateManager.h"
#include "storage/StorageManager.h"
#include "drivers/LedDriver.h"
#include "effects/EffectManager.h"
#include "network/BleManager.h"
#include "network/WifiManager.h"
#include "network/MqttManager.h"
#include "network/OtaManager.h"

#define TAG "MAIN"

// Task handles
TaskHandle_t ledTaskHandle = nullptr;

// Dedicated FreeRTOS Task for smooth LED rendering on Core 1
void ledTask(void* parameter) {
    LOG_I(TAG, "LED Render Task started on Core %d", xPortGetCoreID());
    TickType_t lastWakeTime = xTaskGetTickCount();
    const TickType_t frameDelay = pdMS_TO_TICKS(16); // ~60 FPS

    for (;;) {
        EffectManager::getInstance().update();
        vTaskDelayUntil(&lastWakeTime, frameDelay);
    }
}

void setup() {
    // 1. Initialize Logger
    Logger::init(115200);
    LOG_I(TAG, "=== %s v%s Starting ===", DEVICE_NAME, FIRMWARE_VERSION);

    // 2. Initialize State / Flash Storage
    StateManager::getInstance().init();

    // 3. Initialize File System Storage (SPIFFS for Character Projections - Flow 2)
    StorageManager::getInstance().init();

    // 4. Initialize Hardware Drivers (LED Matrix HUB75)
    LedDriver::getInstance().init();

    // 5. Initialize Effects
    EffectManager::getInstance().init();

    // 5. Initialize Connectivity (BLE, WiFi, MQTT, OTA)
    BleManager::getInstance().init();
    WifiManager::getInstance().init();
    MqttManager::getInstance().init();
    OtaManager::getInstance().init();

    // 6. Spawn high-priority LED Render Task on Core 1
    xTaskCreatePinnedToCore(
        ledTask,
        "LedTask",
        STACK_SIZE_LED_TASK,
        nullptr,
        2,                  // Priority
        &ledTaskHandle,
        CORE_LED_RENDER     // Core 1
    );

    LOG_I(TAG, "System initialization complete. Running main loop on Core %d...", xPortGetCoreID());
}

void loop() {
    // Background polling on Core 0
    BleManager::getInstance().update();
    WifiManager::getInstance().update();
    MqttManager::getInstance().update();
    OtaManager::getInstance().update();

    vTaskDelay(pdMS_TO_TICKS(10));
}
