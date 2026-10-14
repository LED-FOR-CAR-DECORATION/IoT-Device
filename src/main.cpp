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
#include "core/CarSignalService.h"

#define TAG "MAIN"

// Task handles
TaskHandle_t ledTaskHandle = nullptr;

// Emergency Stop (E-Stop) State & Debounce
static volatile bool g_isEmergencyStopped = false;
static volatile unsigned long g_lastDebounceTime = 0;

void IRAM_ATTR handleEmergencyStopISR() {
    unsigned long now = millis();
    if (now - g_lastDebounceTime > 50) {
        // LOW = Button pressed (closed to GND), HIGH = Button released
        g_isEmergencyStopped = (digitalRead(PIN_E_STOP) == LOW);
        g_lastDebounceTime = now;
    }
}

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

    // 2. Configure Hardware Emergency Stop (E-Stop)
    pinMode(PIN_E_STOP, INPUT_PULLUP);
    g_isEmergencyStopped = (digitalRead(PIN_E_STOP) == LOW);
    StateManager::getInstance().setEmergencyStopped(g_isEmergencyStopped);
    attachInterrupt(digitalPinToInterrupt(PIN_E_STOP), handleEmergencyStopISR, CHANGE);
    if (g_isEmergencyStopped) {
        LOG_W("E_STOP", "System started with Emergency Stop ACTIVE (Button Pressed)!");
    } else {
        LOG_I("E_STOP", "Emergency Stop button initialized on GPIO %d (Status: READY)", PIN_E_STOP);
    }

    // 3. Initialize State / Flash Storage
    StateManager::getInstance().init();

    // 4. Initialize File System Storage (SPIFFS for Character Projections - Flow 2)
    StorageManager::getInstance().init();

    // 5. Initialize Hardware Drivers (LED Matrix HUB75)
    LedDriver::getInstance().init();

    // 6. Initialize Effects
    EffectManager::getInstance().init();

    // 7. Initialize Connectivity (BLE, WiFi, MQTT, OTA)
    BleManager::getInstance().init();
    WifiManager::getInstance().init();
    MqttManager::getInstance().init();
    OtaManager::getInstance().init();

    // 8. Initialize Automotive Signal Service (Simulator & Signal Processor)
    CarSignalService::getInstance().init();

    // 9. Spawn high-priority LED Render Task on Core 1
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
    // 1. Process Car Signals & Serial Simulator
    CarSignalService::getInstance().update();

    // 2. Background polling on Core 0
    BleManager::getInstance().update();
    WifiManager::getInstance().update();
    MqttManager::getInstance().update();
    OtaManager::getInstance().update();

    // Periodic Telemetry Heartbeat (5s) for BLE and MQTT
    static unsigned long lastTelemetryTime = 0;
    unsigned long now = millis();
    if (now - lastTelemetryTime >= 5000) {
        lastTelemetryTime = now;
        String telemetry = StateManager::getInstance().getTelemetryJson();
        if (BleManager::getInstance().isConnected()) {
            BleManager::getInstance().sendStatusNotification(telemetry);
        }
        MqttManager::getInstance().publishStatus(telemetry.c_str());
    }

    vTaskDelay(pdMS_TO_TICKS(10));
}