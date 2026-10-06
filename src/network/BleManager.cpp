#include "BleManager.h"
#include "config.h"
#include "core/StateManager.h"
#include "effects/EffectManager.h"
#include "utils/Logger.h"

#define TAG "BLE"

BleManager& BleManager::getInstance() {
    static BleManager instance;
    return instance;
}

BleManager::BleManager()
    : pServer(nullptr), pControlCharacteristic(nullptr),
      pStatusCharacteristic(nullptr), deviceConnected(false),
      advertisingStartTime(0), isFastAdvertising(false) {}

void BleManager::ServerCallbacks::onConnect(NimBLEServer* pServer, ble_gap_conn_desc* desc) {
    parent->deviceConnected = true;
    LOG_I(TAG, "Android smartphone connected over BLE (conn_handle: %d)", desc->conn_handle);
    // Request fast connection parameters: 15ms - 30ms interval, latency 0, timeout 4s
    pServer->updateConnParams(desc->conn_handle, 12, 24, 0, 400);
}

void BleManager::ServerCallbacks::onDisconnect(NimBLEServer* pServer) {
    parent->deviceConnected = false;
    LOG_I(TAG, "Client disconnected, restarting advertising...");
    parent->configureAdvertising(true);
}

void BleManager::CharacteristicCallbacks::onWrite(NimBLECharacteristic* pCharacteristic) {
    std::string rxValue = pCharacteristic->getValue();
    if (!rxValue.empty()) {
        parent->handleIncomingCommand(
            reinterpret_cast<const uint8_t*>(rxValue.data()),
            rxValue.size()
        );
    }
}

void BleManager::configureAdvertising(bool fast) {
    NimBLEAdvertising* pAdvertising = NimBLEDevice::getAdvertising();
    pAdvertising->stop();

    if (fast) {
        // Fast advertising: 20ms - 30ms interval (32 - 48 units of 0.625ms)
        // Enables Android to scan and connect in < 300ms upon entering the car
        pAdvertising->setMinInterval(32);
        pAdvertising->setMaxInterval(48);
        isFastAdvertising = true;
    } else {
        // Normal advertising: 100ms - 150ms interval (160 - 240 units of 0.625ms)
        pAdvertising->setMinInterval(160);
        pAdvertising->setMaxInterval(240);
        isFastAdvertising = false;
    }

    // Set preferred connection intervals in advertising data for Android OS
    pAdvertising->setMinPreferred(12); // 15ms
    pAdvertising->setMaxPreferred(24); // 30ms

    advertisingStartTime = millis();
    pAdvertising->start();
    LOG_I(TAG, "BLE advertising (%s mode)", fast ? "FAST 20ms" : "NORMAL 100ms");
}

void BleManager::init() {
    NimBLEDevice::init(DEVICE_NAME);
    NimBLEDevice::setPower(ESP_PWR_LVL_P9); // +9dBm max TX power for cabin coverage
    NimBLEDevice::setMTU(256);              // High MTU support

    pServer = NimBLEDevice::createServer();
    pServer->setCallbacks(new ServerCallbacks(this));

    NimBLEService* pService = pServer->createService(BLE_SERVICE_UUID);

    // Control Characteristic (Write and Write-Without-Response for high-speed commands)
    pControlCharacteristic = pService->createCharacteristic(
        BLE_CHAR_CONTROL_UUID,
        NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::WRITE_NR
    );
    pControlCharacteristic->setCallbacks(new CharacteristicCallbacks(this));

    // Status Characteristic (Read / Notify)
    pStatusCharacteristic = pService->createCharacteristic(
        BLE_CHAR_STATUS_UUID,
        NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY
    );

    pService->start();

    // Start with Fast Advertising for immediate car startup discovery
    configureAdvertising(true);

    LOG_I(TAG, "BLE initialized and advertising as '%s'", DEVICE_NAME);
}

void BleManager::handleIncomingCommand(const uint8_t* data, size_t length) {
    if (data == nullptr || length == 0) return;

    // Check if packet matches 6-byte binary protocol frame: [0xAA, CMD, P1, P2, P3, 0x55]
    if (length == 6 && data[0] == PROTOCOL_START_BYTE && data[5] == PROTOCOL_END_BYTE) {
        handleBinaryFrame(data, length);
    } else {
        // Fallback / compatibility JSON string
        std::string jsonStr(reinterpret_cast<const char*>(data), length);
        handleJsonPayload(jsonStr);
    }
}

void BleManager::handleBinaryFrame(const uint8_t* frame, size_t length) {
    uint8_t opcode = frame[1];
    uint8_t p1 = frame[2];
    uint8_t p2 = frame[3];
    uint8_t p3 = frame[4];

    StateManager& state = StateManager::getInstance();

    switch (opcode) {
        case BLE_CMD_POWER:
            LOG_D(TAG, "Binary CMD: Power -> %d", p1);
            state.setPower(p1 != 0);
            break;

        case BLE_CMD_BRIGHTNESS:
            LOG_D(TAG, "Binary CMD: Brightness -> %d", p1);
            state.setBrightness(p1);
            break;

        case BLE_CMD_SPEED:
            LOG_D(TAG, "Binary CMD: Speed -> %d", p1);
            state.setSpeed(p1);
            break;

        case BLE_CMD_MODE:
            LOG_D(TAG, "Binary CMD: Mode -> %d", p1);
            state.setMode(static_cast<EffectMode>(p1));
            EffectManager::getInstance().setMode(static_cast<EffectMode>(p1));
            break;

        case BLE_CMD_COLOR_RGB:
            LOG_D(TAG, "Binary CMD: RGB -> (%d, %d, %d)", p1, p2, p3);
            state.setPrimaryColor(p1, p2, p3);
            break;

        case BLE_CMD_MIC_SENSITIVITY: {
            LOG_D(TAG, "Binary CMD: Sensitivity -> %d", p1);
            DeviceSettings s = state.getSettings();
            s.micSensitivity = p1;
            state.updateSettings(s);
            break;
        }

        default:
            LOG_W(TAG, "Unknown binary opcode: 0x%02X", opcode);
            break;
    }
}

void BleManager::handleJsonPayload(const std::string& commandStr) {
    LOG_D(TAG, "BLE JSON payload: %s", commandStr.c_str());

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, commandStr);
    if (error) {
        LOG_W(TAG, "Failed to parse JSON command: %s", error.c_str());
        return;
    }

    StateManager& state = StateManager::getInstance();

    if (doc["power"].is<bool>()) {
        state.setPower(doc["power"].as<bool>());
    }
    if (doc["brightness"].is<uint8_t>() || doc["brightness"].is<int>()) {
        state.setBrightness(doc["brightness"].as<uint8_t>());
    }
    if (doc["speed"].is<uint8_t>() || doc["speed"].is<int>()) {
        state.setSpeed(doc["speed"].as<uint8_t>());
    }
    if (doc["mode"].is<uint8_t>() || doc["mode"].is<int>()) {
        uint8_t m = doc["mode"].as<uint8_t>();
        state.setMode(static_cast<EffectMode>(m));
        EffectManager::getInstance().setMode(static_cast<EffectMode>(m));
    }
    if (doc["color"].is<JsonObject>()) {
        uint8_t r = doc["color"]["r"] | 255;
        uint8_t g = doc["color"]["g"] | 255;
        uint8_t b = doc["color"]["b"] | 255;
        state.setPrimaryColor(r, g, b);
    }
}

void BleManager::sendStatusNotification(const String& jsonStatus) {
    if (deviceConnected && pStatusCharacteristic) {
        pStatusCharacteristic->setValue(
            reinterpret_cast<const uint8_t*>(jsonStatus.c_str()),
            jsonStatus.length()
        );
        pStatusCharacteristic->notify();
    }
}

bool BleManager::isConnected() const {
    return deviceConnected;
}

void BleManager::update() {
    // If not connected and fast advertising has exceeded 30 seconds, transition to normal advertising
    if (!deviceConnected && isFastAdvertising && (millis() - advertisingStartTime > 30000)) {
        LOG_I(TAG, "30s fast advertising expired, switching to normal advertising");
        configureAdvertising(false);
    }
}

