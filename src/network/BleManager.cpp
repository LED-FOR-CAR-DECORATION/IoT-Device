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
      pStatusCharacteristic(nullptr), deviceConnected(false) {}

void BleManager::ServerCallbacks::onConnect(NimBLEServer* pServer) {
    parent->deviceConnected = true;
    LOG_I(TAG, "Client smartphone connected over BLE");
}

void BleManager::ServerCallbacks::onDisconnect(NimBLEServer* pServer) {
    parent->deviceConnected = false;
    LOG_I(TAG, "Client disconnected, restarting advertising...");
    NimBLEDevice::startAdvertising();
}

void BleManager::CharacteristicCallbacks::onWrite(NimBLECharacteristic* pCharacteristic) {
    std::string rxValue = pCharacteristic->getValue();
    if (!rxValue.empty()) {
        parent->handleIncomingCommand(rxValue);
    }
}

void BleManager::init() {
    NimBLEDevice::init(DEVICE_NAME);
    NimBLEDevice::setPower(ESP_PWR_LVL_P9); // Max TX power for car cabin coverage

    pServer = NimBLEDevice::createServer();
    pServer->setCallbacks(new ServerCallbacks(this));

    NimBLEService* pService = pServer->createService(BLE_SERVICE_UUID);

    // Control Characteristic (Write)
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

    NimBLEAdvertising* pAdvertising = NimBLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(BLE_SERVICE_UUID);
    pAdvertising->setName(DEVICE_NAME);
    pAdvertising->start();

    LOG_I(TAG, "BLE initialized and advertising as '%s'", DEVICE_NAME);
}

void BleManager::handleIncomingCommand(const std::string& commandStr) {
    LOG_D(TAG, "BLE Command payload: %s", commandStr.c_str());

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
        pStatusCharacteristic->setValue(reinterpret_cast<const uint8_t*>(jsonStatus.c_str()), jsonStatus.length());
        pStatusCharacteristic->notify();
    }
}

bool BleManager::isConnected() const {
    return deviceConnected;
}

void BleManager::update() {
    // BLE background house-keeping if needed
}
