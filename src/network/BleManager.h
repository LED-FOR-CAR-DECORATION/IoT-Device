#pragma once

#include <NimBLEDevice.h>
#include <ArduinoJson.h>

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

    void handleIncomingCommand(const std::string& commandStr);

    class ServerCallbacks : public NimBLEServerCallbacks {
    public:
        ServerCallbacks(BleManager* parent) : parent(parent) {}
        void onConnect(NimBLEServer* pServer) override;
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
