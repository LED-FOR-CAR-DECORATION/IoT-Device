#pragma once

#include <NimBLEDevice.h>
#include <ArduinoJson.h>

// Binary protocol framing bytes
#define PROTOCOL_START_BYTE 0xAA
#define PROTOCOL_END_BYTE   0x55

// Binary command opcodes (High-speed 6-byte frames)
enum BleBinaryOpcode : uint8_t {
    BLE_CMD_POWER           = 0x01,
    BLE_CMD_BRIGHTNESS      = 0x02,
    BLE_CMD_SPEED           = 0x03,
    BLE_CMD_MODE            = 0x04,
    BLE_CMD_COLOR_RGB       = 0x05,
    BLE_CMD_MIC_SENSITIVITY = 0x06
};

class BleManager {
public:
    static BleManager& getInstance();

    void init();
    void update();
    bool isConnected() const;
    void sendStatusNotification(const String& jsonStatus);

private:
    BleManager();

    NimBLEServer* pServer;
    NimBLECharacteristic* pControlCharacteristic;
    NimBLECharacteristic* pStatusCharacteristic;
    bool deviceConnected;
    unsigned long advertisingStartTime;
    bool isFastAdvertising;

    void handleIncomingCommand(const uint8_t* data, size_t length);
    void handleBinaryFrame(const uint8_t* frame, size_t length);
    void handleJsonPayload(const std::string& commandStr);
    void configureAdvertising(bool fast);

    class ServerCallbacks : public NimBLEServerCallbacks {
    public:
        ServerCallbacks(BleManager* parent) : parent(parent) {}
        void onConnect(NimBLEServer* pServer, ble_gap_conn_desc* desc) override;
        void onDisconnect(NimBLEServer* pServer) override;
    private:
        BleManager* parent;
    };

    class CharacteristicCallbacks : public NimBLECharacteristicCallbacks {
    public:
        CharacteristicCallbacks(BleManager* parent) : parent(parent) {}
        void onWrite(NimBLECharacteristic* pCharacteristic) override;
    private:
        BleManager* parent;
    };
};
