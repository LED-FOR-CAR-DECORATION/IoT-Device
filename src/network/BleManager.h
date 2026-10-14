#pragma once

#include <NimBLEDevice.h>
#include <ArduinoJson.h>

// Binary protocol framing bytes
#define PROTOCOL_START_BYTE 0xAA
#define PROTOCOL_END_BYTE   0x55

// Binary command opcodes
enum BleBinaryOpcode : uint8_t {
    BLE_CMD_POWER              = 0x01,
    BLE_CMD_BRIGHTNESS         = 0x02,
    BLE_CMD_SPEED              = 0x03,
    BLE_CMD_MODE               = 0x04,
    BLE_CMD_COLOR_RGB          = 0x05,
    BLE_CMD_MIC_SENSITIVITY    = 0x06,

    // Flow 2: Character / Pixel Art Projection Opcodes
    BLE_CMD_PROJECTION_START   = 0x10, // Metadata: frameCount, width, height, delayMs
    BLE_CMD_PROJECTION_CHUNK   = 0x11, // Data payload chunk
    BLE_CMD_PROJECTION_FINISH  = 0x12, // Finalize, store to SPIFFS, and activate
    BLE_CMD_DRAW_PIXEL         = 0x13, // Interactive live pencil canvas: x, y, r, g, b
    BLE_CMD_DRAW_BATCH         = 0x14, // Batch live pencil canvas: count, [x, y, r, g, b] * count
    BLE_CMD_CANVAS_CLEAR       = 0x15, // Switch to blank black board for live drawing
    BLE_CMD_CANVAS_FILL        = 0x16, // Fill entire canvas with color: r, g, b
    BLE_CMD_DRAW_LINE          = 0x17, // Interactive Line: x0, y0, x1, y1, r, g, b, isCommit
    BLE_CMD_DRAW_RECT          = 0x18, // Interactive Rect: x0, y0, x1, y1, r, g, b, isCommit
    BLE_CMD_FLOOD_FILL         = 0x19  // Bucket Flood Fill: x, y, r, g, b
};

#include "storage/StorageManager.h"
#include <vector>
#include <string>

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
    uint16_t currentConnHandle;
    unsigned long connectedTime;
    bool connParamsUpdated;

    // Buffer for chunked pixel art / animation transfer (Flow 2)
    ProjectionHeader transferHeader;
    std::vector<uint8_t> transferBuffer;
    bool isReceivingProjection;
    size_t expectedTransferBytes;

    // Accumulator buffer for fragmented JSON packets across MTU boundaries
    std::string jsonRxBuffer;
    unsigned long lastJsonRxMillis;

    void handleIncomingCommand(const uint8_t* data, size_t length);
    void handleBinaryFrame(const uint8_t* frame, size_t length);
    void appendAndProcessJsonChunk(const uint8_t* data, size_t length);
    void handleJsonPayload(const std::string& commandStr);
    void handleProjectionStart(const uint8_t* frame, size_t length);
    void handleProjectionChunk(const uint8_t* frame, size_t length);
    void handleProjectionFinish(const uint8_t* frame, size_t length);
    void handleDrawPixel(const uint8_t* frame, size_t length);
    void handleDrawBatch(const uint8_t* frame, size_t length);
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
