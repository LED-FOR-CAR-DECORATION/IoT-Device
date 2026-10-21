#include "BleManager.h"
#include <Preferences.h>
#include "config.h"
#include "core/StateManager.h"
#include "effects/EffectManager.h"
#include "network/WifiManager.h"
#include "storage/StorageManager.h"
#include "utils/ColorUtils.h"
#include "utils/Logger.h"

#define TAG "BLE"

BleManager& BleManager::getInstance() {
    static BleManager instance;
    return instance;
}

BleManager::BleManager()
    : pServer(nullptr), pControlCharacteristic(nullptr),
      pStatusCharacteristic(nullptr), deviceConnected(false),
      advertisingStartTime(0), isFastAdvertising(false),
      currentConnHandle(0), connectedTime(0), connParamsUpdated(false),
      isReceivingProjection(false), expectedTransferBytes(0),
      lastJsonRxMillis(0) {}

void BleManager::ServerCallbacks::onConnect(NimBLEServer* pServer, ble_gap_conn_desc* desc) {
    parent->deviceConnected = true;
    parent->currentConnHandle = desc->conn_handle;
    parent->connectedTime = millis();
    parent->connParamsUpdated = false;
    LOG_I(TAG, "Smartphone connected over BLE (conn_handle: %d)", desc->conn_handle);
    // DO NOT call pServer->updateConnParams() immediately here!
    // Updating connection parameters during initial GATT service discovery triggers GATT 133 on Android devices.
}

void BleManager::ServerCallbacks::onDisconnect(NimBLEServer* pServer) {
    parent->deviceConnected = false;
    parent->connParamsUpdated = false;
    parent->jsonRxBuffer.clear();
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

    NimBLEAdvertisementData advertisement;
    advertisement.setFlags(0x06);
    advertisement.setCompleteServices(NimBLEUUID(BLE_SERVICE_UUID));
    NimBLEAdvertisementData response;
    response.setName(deviceName.c_str());
    pAdvertising->setAdvertisementData(advertisement);
    pAdvertising->setScanResponseData(response);

    if (fast) {
        // Fast advertising: 20ms - 30ms interval (32 - 48 units of 0.625ms)
        pAdvertising->setMinInterval(32);
        pAdvertising->setMaxInterval(48);
        isFastAdvertising = true;
    } else {
        // Normal advertising: 100ms - 150ms interval (160 - 240 units of 0.625ms)
        pAdvertising->setMinInterval(160);
        pAdvertising->setMaxInterval(240);
        isFastAdvertising = false;
    }

    // Set preferred connection intervals in advertising data for smartphone OS (units of 1.25ms)
    pAdvertising->setMinPreferred(16); // 20ms
    pAdvertising->setMaxPreferred(32); // 40ms

    advertisingStartTime = millis();
    pAdvertising->start();
    LOG_I(TAG, "BLE advertising (%s mode)", fast ? "FAST 20ms" : "NORMAL 100ms");
}

void BleManager::init() {
    Preferences identity;
    deviceName = DEVICE_NAME;
    if (identity.begin("ble_identity", true)) {
        String stored = identity.getString("name", DEVICE_NAME);
        if (stored.length() > 0 && stored.length() <= 29) deviceName = stored;
        identity.end();
    }
    NimBLEDevice::init(deviceName.c_str());
    NimBLEDevice::setPower(ESP_PWR_LVL_P9); // +9dBm max TX power for cabin coverage
    NimBLEDevice::setMTU(256);              // Support up to 256 MTU when requested by client

    pServer = NimBLEDevice::createServer();
    pServer->setCallbacks(new ServerCallbacks(this));

    NimBLEService* pService = pServer->createService(BLE_SERVICE_UUID);

    // Control Characteristic (Write and Write-Without-Response for high-throughput commands)
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

    // Configure Advertising packet with Service UUID and Scan Response
    NimBLEAdvertising* pAdvertising = NimBLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(BLE_SERVICE_UUID);
    pAdvertising->setScanResponse(true);

    // Start with Fast Advertising for immediate startup discovery
    configureAdvertising(true);

    LOG_I(TAG, "BLE initialized and advertising as '%s'", deviceName.c_str());
}

void BleManager::handleIncomingCommand(const uint8_t* data, size_t length) {
    if (data == nullptr || length == 0) return;

    // Check if packet matches binary protocol frame: [0xAA, ..., 0x55]
    if (length >= 6 && data[0] == PROTOCOL_START_BYTE && data[length - 1] == PROTOCOL_END_BYTE) {
        handleBinaryFrame(data, length);
    } else {
        // Handle chunked or single-packet JSON text across MTU boundaries
        appendAndProcessJsonChunk(data, length);
    }
}

void BleManager::handleBinaryFrame(const uint8_t* frame, size_t length) {
    uint8_t opcode = frame[1];
    StateManager& state = StateManager::getInstance();

    // Verify Checksum if present:
    // Standard frame with Checksum (7 bytes): [0xAA, cmd, p1, p2, p3, checksum, 0x55]
    // checksum = cmd ^ p1 ^ p2 ^ p3
    if (length == 7) {
        uint8_t expectedChecksum = frame[1] ^ frame[2] ^ frame[3] ^ frame[4];
        if (frame[5] != expectedChecksum) {
            LOG_W(TAG, "Binary frame checksum error! Expected 0x%02X, got 0x%02X", expectedChecksum, frame[5]);
            return;
        }
    }

    switch (opcode) {
        case BLE_CMD_POWER:
            if (length >= 6) {
                LOG_I(TAG, "Binary CMD: Power -> %s", frame[2] ? "ON" : "OFF");
                state.setPower(frame[2] != 0);
            }
            break;

        case BLE_CMD_BRIGHTNESS:
            if (length >= 6) {
                LOG_I(TAG, "Binary CMD: Brightness -> %d", frame[2]);
                state.setBrightness(frame[2]);
            }
            break;

        case BLE_CMD_SPEED:
            if (length >= 6) {
                LOG_I(TAG, "Binary CMD: Speed -> %d", frame[2]);
                state.setSpeed(frame[2]);
            }
            break;

        case BLE_CMD_MODE:
            if (length >= 6) {
                uint8_t m = frame[2];
                LOG_I(TAG, "Binary CMD: Mode -> %d", m);
                EffectMode newMode = static_cast<EffectMode>(m);
                state.setMode(newMode);
                EffectManager::getInstance().setMode(newMode);
                if (newMode == EffectMode::WELCOME_CAR || newMode == EffectMode::CUSTOM_PRESET) {
                    EffectManager::getInstance().getCustomEffect().loadFromFlash();
                }
            }
            break;

        case BLE_CMD_COLOR_RGB:
            if (length >= 6) {
                LOG_I(TAG, "Binary CMD: RGB -> (%d, %d, %d)", frame[2], frame[3], frame[4]);
                state.setPrimaryColor(frame[2], frame[3], frame[4]);
                state.setMode(EffectMode::STATIC_COLOR);
                EffectManager::getInstance().setMode(EffectMode::STATIC_COLOR);
            }
            break;

        case BLE_CMD_MIC_SENSITIVITY:
            if (length >= 6) {
                LOG_I(TAG, "Binary CMD: Mic Sensitivity -> %d", frame[2]);
            }
            break;

        // Flow 2: Character / Pixel Art Projection opcodes
        case BLE_CMD_PROJECTION_START:
            handleProjectionStart(frame, length);
            break;

        case BLE_CMD_PROJECTION_CHUNK:
            handleProjectionChunk(frame, length);
            break;

        case BLE_CMD_PROJECTION_FINISH:
            handleProjectionFinish(frame, length);
            break;

        case BLE_CMD_DRAW_PIXEL:
            state.setMode(EffectMode::CUSTOM_PRESET);
            EffectManager::getInstance().setMode(EffectMode::CUSTOM_PRESET);
            handleDrawPixel(frame, length);
            break;

        case BLE_CMD_DRAW_BATCH:
            state.setMode(EffectMode::CUSTOM_PRESET);
            EffectManager::getInstance().setMode(EffectMode::CUSTOM_PRESET);
            handleDrawBatch(frame, length);
            break;

        case BLE_CMD_DRAW_CONFIRMED: {
            // [AA,1A,seqHi,seqLo,count,(x,y,r,g,b)*count,xor,55]
            if (length < 7) break;
            const uint8_t count = frame[4];
            if (count > 12 || length != static_cast<size_t>(7 + count * 5)) break;
            uint8_t checksum = 0;
            for (size_t i = 1; i < length - 2; ++i) checksum ^= frame[i];
            if (checksum != frame[length - 2]) break;
            state.setMode(EffectMode::CUSTOM_PRESET);
            EffectManager::getInstance().setMode(EffectMode::CUSTOM_PRESET);
            auto& canvas = EffectManager::getInstance().getCustomEffect();
            const bool applied = count == 0 ? canvas.resetToBlankCanvas() : canvas.drawLiveBatch(frame + 5, count);
            if (applied) {
                const uint16_t sequence = (frame[2] << 8) | frame[3];
                sendStatusNotification("{\"event\":\"canvas_batch_applied\",\"sequence\":" + String(sequence) + "}");
            }
            break;
        }

        case BLE_CMD_CANVAS_CLEAR:
            LOG_I(TAG, "BLE: Switching to blank black board for live drawing");
            state.setMode(EffectMode::CUSTOM_PRESET);
            EffectManager::getInstance().setMode(EffectMode::CUSTOM_PRESET);
            EffectManager::getInstance().getCustomEffect().resetToBlankCanvas();
            break;

        case BLE_CMD_CANVAS_FILL:
            if (length >= 6) {
                state.setMode(EffectMode::CUSTOM_PRESET);
                EffectManager::getInstance().setMode(EffectMode::CUSTOM_PRESET);
                uint8_t r = frame[2], g = frame[3], b = frame[4];
                EffectManager::getInstance().getCustomEffect().fillCanvas(ColorUtils::color565(r, g, b));
            }
            break;

        case BLE_CMD_DRAW_LINE:
            if (length >= 11) {
                state.setMode(EffectMode::CUSTOM_PRESET);
                EffectManager::getInstance().setMode(EffectMode::CUSTOM_PRESET);
                int16_t x0 = frame[2], y0 = frame[3], x1 = frame[4], y1 = frame[5];
                uint8_t r = frame[6], g = frame[7], b = frame[8];
                bool isCommit = (frame[9] != 0);
                EffectManager::getInstance().getCustomEffect().drawLiveLine(x0, y0, x1, y1, ColorUtils::color565(r, g, b), isCommit);
            }
            break;

        case BLE_CMD_DRAW_RECT:
            if (length >= 11) {
                state.setMode(EffectMode::CUSTOM_PRESET);
                EffectManager::getInstance().setMode(EffectMode::CUSTOM_PRESET);
                int16_t x0 = frame[2], y0 = frame[3], x1 = frame[4], y1 = frame[5];
                uint8_t r = frame[6], g = frame[7], b = frame[8];
                bool isCommit = (frame[9] != 0);
                EffectManager::getInstance().getCustomEffect().drawLiveRect(x0, y0, x1, y1, ColorUtils::color565(r, g, b), isCommit);
            }
            break;

        case BLE_CMD_FLOOD_FILL:
            if (length >= 7) {
                state.setMode(EffectMode::CUSTOM_PRESET);
                EffectManager::getInstance().setMode(EffectMode::CUSTOM_PRESET);
                int16_t x = frame[2], y = frame[3];
                uint8_t r = frame[4], g = frame[5], b = frame[6];
                EffectManager::getInstance().getCustomEffect().floodFill(x, y, ColorUtils::color565(r, g, b));
            }
            break;

        default:
            LOG_W(TAG, "Unknown binary opcode: 0x%02X", opcode);
            break;
    }
}

void BleManager::handleProjectionStart(const uint8_t* frame, size_t length) {
    // Frame: [0xAA, 0x10, frameCount, width, height, delayHigh, delayLow, 0x55] (8 bytes)
    if (length < 8) {
        LOG_E(TAG, "Invalid PROJECTION_START frame size (%u)", (unsigned int)length);
        return;
    }

    transferHeader.magic = PROJECTION_MAGIC;
    transferHeader.version = 1;
    transferHeader.frameCount = frame[2] > 0 ? frame[2] : 1;
    transferHeader.width = frame[3] > 0 ? frame[3] : 64;
    transferHeader.height = frame[4] > 0 ? frame[4] : 32;
    transferHeader.frameDelayMs = (frame[5] << 8) | frame[6];
    if (transferHeader.frameDelayMs == 0) transferHeader.frameDelayMs = 200;
    transferHeader.colorFormat = 0; // RGB565

    expectedTransferBytes = transferHeader.frameCount * transferHeader.width * transferHeader.height * sizeof(uint16_t);
    transferBuffer.clear();
    transferBuffer.reserve(expectedTransferBytes);
    isReceivingProjection = true;

    LOG_I(TAG, "Projection upload started: %dx%d, %d frame(s), delay: %dms, expected %u bytes",
          transferHeader.width, transferHeader.height, transferHeader.frameCount,
          transferHeader.frameDelayMs, (unsigned int)expectedTransferBytes);

    sendStatusNotification("{\"event\":\"upload_started\",\"expected\":" + String((unsigned int)expectedTransferBytes) + "}");
}

void BleManager::handleProjectionChunk(const uint8_t* frame, size_t length) {
    // Frame: [0xAA, 0x11, chunkIdxH, chunkIdxL, dataLen, <DATA...>, 0x55]
    if (!isReceivingProjection) {
        LOG_W(TAG, "PROJECTION_CHUNK received but transfer not started");
        return;
    }

    if (length < 7) {
        LOG_E(TAG, "Malformed chunk packet (%u bytes)", (unsigned int)length);
        return;
    }

    uint16_t chunkIndex = (frame[2] << 8) | frame[3];
    uint8_t dataLen = frame[4];

    if (length < (size_t)(5 + dataLen + 1)) {
        LOG_E(TAG, "Chunk payload length mismatch (declared: %d, frame: %u)", dataLen, (unsigned int)length);
        return;
    }

    const uint8_t* payload = &frame[5];
    transferBuffer.insert(transferBuffer.end(), payload, payload + dataLen);

    LOG_D(TAG, "Chunk #%u: +%u bytes (%u / %u)", chunkIndex, dataLen,
          (unsigned int)transferBuffer.size(), (unsigned int)expectedTransferBytes);
}

void BleManager::handleProjectionFinish(const uint8_t* frame, size_t length) {
    if (!isReceivingProjection) {
        LOG_W(TAG, "PROJECTION_FINISH received but not in transfer mode");
        return;
    }

    isReceivingProjection = false;
    bool activateNow = (length >= 3 && frame[2] != 0);

    LOG_I(TAG, "Projection upload finished. Storing %u bytes to SPIFFS...", (unsigned int)transferBuffer.size());

    bool saved = StorageManager::getInstance().savePattern(
        DEFAULT_PATTERN_PATH, transferHeader, transferBuffer.data(), transferBuffer.size()
    );

    if (saved) {
        EffectManager::getInstance().getCustomEffect().setFrames(
            transferHeader, transferBuffer.data(), transferBuffer.size()
        );

        if (activateNow) {
            StateManager::getInstance().setMode(EffectMode::CUSTOM_PRESET);
            EffectManager::getInstance().setMode(EffectMode::CUSTOM_PRESET);
        }

        LOG_I(TAG, "Flow 2: Stored projection pattern and activated successfully!");
        sendStatusNotification("{\"event\":\"upload_complete\",\"status\":\"ok\",\"mode\":6}");
    } else {
        LOG_E(TAG, "Failed to save projection pattern to SPIFFS");
        sendStatusNotification("{\"event\":\"upload_error\",\"error\":\"flash_write_fail\"}");
    }

    transferBuffer.clear();
}

void BleManager::handleDrawPixel(const uint8_t* frame, size_t length) {
    // Frame 8 bytes (legacy): [0xAA, 0x13, x, y, r, g, b, 0x55]
    // Frame 9 bytes (checksum): [0xAA, 0x13, x, y, r, g, b, checksum, 0x55]
    if (length < 8) return;

    if (length == 9) {
        uint8_t expectedChecksum = frame[1] ^ frame[2] ^ frame[3] ^ frame[4] ^ frame[5] ^ frame[6];
        if (frame[7] != expectedChecksum) {
            LOG_W(TAG, "Draw pixel checksum mismatch! Calc: 0x%02X, Got: 0x%02X", expectedChecksum, frame[7]);
            return;
        }
    }

    int16_t x = frame[2];
    int16_t y = frame[3];
    uint8_t r = frame[4];
    uint8_t g = frame[5];
    uint8_t b = frame[6];

    uint16_t col565 = ColorUtils::color565(r, g, b);
    EffectManager::getInstance().getCustomEffect().drawLivePixel(x, y, col565);
}

void BleManager::handleDrawBatch(const uint8_t* frame, size_t length) {
    // Frame with checksum: [0xAA, 0x14, count, <x, y, r, g, b> * count, checksum, 0x55] (length = count * 5 + 5)
    // Frame without checksum: [0xAA, 0x14, count, <x, y, r, g, b> * count, 0x55] (length = count * 5 + 4)
    if (length < 5) return;
    uint8_t count = frame[2];
    if (count == 0) return;

    size_t expectedLenWithCsum = (size_t)count * 5 + 5;
    size_t expectedLenWithoutCsum = (size_t)count * 5 + 4;

    if (length == expectedLenWithCsum) {
        uint8_t csum = 0;
        for (size_t i = 1; i <= (size_t)(2 + count * 5); i++) {
            csum ^= frame[i];
        }
        if (frame[3 + count * 5] != csum) {
            LOG_W(TAG, "Draw batch checksum mismatch (calc 0x%02X, got 0x%02X)", csum, frame[3 + count * 5]);
            return;
        }
    } else if (length != expectedLenWithoutCsum) {
        LOG_W(TAG, "Invalid draw batch packet size (%u bytes for %u points)", (unsigned int)length, count);
        return;
    }

    EffectManager::getInstance().getCustomEffect().drawLiveBatch(frame + 3, count);
}

void BleManager::appendAndProcessJsonChunk(const uint8_t* data, size_t length) {
    unsigned long now = millis();
    // Timeout: if previous incomplete buffer is older than 1000ms, discard stale fragments
    if (!jsonRxBuffer.empty() && (now - lastJsonRxMillis > 1000)) {
        LOG_W(TAG, "JSON stream timeout (>1000ms), dropping %u stale bytes", (unsigned int)jsonRxBuffer.size());
        jsonRxBuffer.clear();
    }
    lastJsonRxMillis = now;

    // Safety guard against memory exhaustion
    if (jsonRxBuffer.size() + length > 2048) {
        LOG_E(TAG, "JSON buffer overflow (>2048 bytes), resetting");
        jsonRxBuffer.clear();
        return;
    }

    jsonRxBuffer.append(reinterpret_cast<const char*>(data), length);

    // Skip leading whitespace if starting fresh
    size_t firstChar = jsonRxBuffer.find_first_not_of(" \t\r\n");
    if (firstChar == std::string::npos) {
        jsonRxBuffer.clear();
        return;
    }
    if (firstChar > 0) {
        jsonRxBuffer.erase(0, firstChar);
    }

    // Fast check: JSON packet must start with '{' or '['
    if (jsonRxBuffer[0] != '{' && jsonRxBuffer[0] != '[') {
        LOG_W(TAG, "Disregarding non-JSON fragment (starts with 0x%02X)", (uint8_t)jsonRxBuffer[0]);
        jsonRxBuffer.clear();
        return;
    }

    // Check completion criteria:
    // 1. Explicit line break '\n' or null terminator
    // 2. Or balanced brace / bracket depth (self-delimited JSON)
    bool potentialComplete = false;
    if (jsonRxBuffer.back() == '\n' || jsonRxBuffer.back() == '\0') {
        potentialComplete = true;
    } else {
        int braceDepth = 0;
        int bracketDepth = 0;
        bool inString = false;
        bool escape = false;

        for (char c : jsonRxBuffer) {
            if (escape) {
                escape = false;
                continue;
            }
            if (c == '\\') {
                escape = true;
                continue;
            }
            if (c == '"') {
                inString = !inString;
                continue;
            }
            if (!inString) {
                if (c == '{') braceDepth++;
                else if (c == '}') braceDepth--;
                else if (c == '[') bracketDepth++;
                else if (c == ']') bracketDepth--;
            }
        }

        if (braceDepth == 0 && bracketDepth == 0 && !jsonRxBuffer.empty()) {
            potentialComplete = true;
        }
    }

    if (potentialComplete) {
        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, jsonRxBuffer);
        if (error == DeserializationError::Ok) {
            LOG_I(TAG, "Assembled complete JSON payload (%u bytes across MTU chunks)", (unsigned int)jsonRxBuffer.size());
            handleJsonPayload(jsonRxBuffer);
            jsonRxBuffer.clear();
        } else if (error == DeserializationError::IncompleteInput) {
            LOG_D(TAG, "JSON chunk incomplete, waiting for subsequent GATT write...");
        } else {
            LOG_W(TAG, "JSON syntax error (%s). Clearing accumulator.", error.c_str());
            jsonRxBuffer.clear();
        }
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

    const String command = doc["command"] | "";
    if (command == "rename_device" || command == "get_device_info") {
        JsonDocument reply;
        reply["request_id"] = doc["request_id"] | "";
        reply["event"] = "device_name";
        if (command == "rename_device") {
            String proposed = doc["name"] | "";
            proposed.trim();
            bool valid = proposed.length() > 0 && proposed.length() <= 29;
            for (size_t i = 0; i < proposed.length(); ++i)
                if ((uint8_t)proposed[i] < 32 || (uint8_t)proposed[i] == 127) valid = false;
            bool saved = false;
            if (valid) {
                Preferences identity;
                if (identity.begin("ble_identity", false)) {
                    saved = identity.putString("name", proposed) == proposed.length();
                    identity.end();
                }
            }
            if (saved) {
                deviceName = proposed;
                NimBLEDevice::setDeviceName(deviceName.c_str());
            } else {
                reply["error"] = valid ? "Không lưu được tên trên LED" : "Tên phải có 1–29 byte UTF-8 và không có ký tự điều khiển";
            }
        }
        reply["name"] = deviceName;
        String payload;
        serializeJson(reply, payload);
        sendStatusNotification(payload);
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
        EffectMode newMode = static_cast<EffectMode>(m);
        state.setMode(newMode);
        EffectManager::getInstance().setMode(newMode);
        if (newMode == EffectMode::WELCOME_CAR || newMode == EffectMode::CUSTOM_PRESET) {
            EffectManager::getInstance().getCustomEffect().loadFromFlash();
        }
    }
    if (doc["color"].is<JsonObject>()) {
        uint8_t r = doc["color"]["r"] | 255;
        uint8_t g = doc["color"]["g"] | 255;
        uint8_t b = doc["color"]["b"] | 255;
        state.setPrimaryColor(r, g, b);
    }

    // Marquee / Scrolling Text JSON Commands:
    bool isMarquee = false;
    if (doc["type"].is<const char*>() && strcmp(doc["type"].as<const char*>(), "marquee") == 0) isMarquee = true;
    if (doc["command"].is<const char*>() && strcmp(doc["command"].as<const char*>(), "marquee") == 0) isMarquee = true;
    if (doc["text"].is<const char*>()) isMarquee = true;

    if (isMarquee) {
        const char* textStr = doc["text"] | "CAR LED";
        uint8_t r = 255, g = 215, b = 0;
        if (doc["color"].is<JsonObject>()) {
            r = doc["color"]["r"] | 255;
            g = doc["color"]["g"] | 215;
            b = doc["color"]["b"] | 0;
        }
        uint8_t speed = doc["speed"] | 60;
        uint8_t size = doc["size"] | 1;
        int16_t yPos = doc["y_pos"] | -1;
        bool rainbow = doc["rainbow"] | false;
        const char* modeStr = doc["mode"] | "scroll_left";

        EffectManager::getInstance().getMarqueeEffect().configure(
            String(textStr), RGBColor(r, g, b), speed, size, yPos, rainbow, String(modeStr)
        );

        state.setMode(EffectMode::MARQUEE_TEXT);
        EffectManager::getInstance().setMode(EffectMode::MARQUEE_TEXT);

        LOG_I(TAG, "BLE: Activated Marquee Text: '%s' (sz: %d, spd: %d, rainbow: %d, mode: %s)",
              textStr, size, speed, rainbow, modeStr);
        sendStatusNotification("{\"event\":\"marquee_active\",\"text\":\"" + String(textStr) + "\"}");
        return;
    }

    // Flow 2 JSON Commands:
    if (doc["command"].is<const char*>()) {
        const char* cmd = doc["command"].as<const char*>();
        if (strcmp(cmd, "reload_projection") == 0) {
            EffectManager::getInstance().getCustomEffect().loadFromFlash();
            state.setMode(EffectMode::CUSTOM_PRESET);
            EffectManager::getInstance().setMode(EffectMode::CUSTOM_PRESET);
            sendStatusNotification("{\"event\":\"projection_reloaded\",\"mode\":6}");
        } else if (strcmp(cmd, "clear_projection") == 0) {
            StorageManager::getInstance().deletePattern();
            EffectManager::getInstance().getCustomEffect().clearFrames();
            sendStatusNotification("{\"event\":\"projection_cleared\"}");
        } else if (strcmp(cmd, "draw_pixel") == 0) {
            int x = doc["x"] | 0;
            int y = doc["y"] | 0;
            uint8_t r = doc["r"] | 255;
            uint8_t g = doc["g"] | 255;
            uint8_t b = doc["b"] | 255;
            EffectManager::getInstance().getCustomEffect().drawLivePixel(x, y, ColorUtils::color565(r, g, b));
        } else if (strcmp(cmd, "wifi_config") == 0 || strcmp(cmd, "connect_wifi") == 0) {
            const char* ssid = doc["ssid"] | "";
            const char* pass = doc["password"] | (doc["pass"] | "");
            if (strlen(ssid) > 0) {
                LOG_I(TAG, "BLE: Received Wi-Fi credentials for SSID '%s'", ssid);
                WifiManager::getInstance().connectNewWifi(ssid, pass);
                sendStatusNotification("{\"event\":\"wifi_connecting\",\"ssid\":\"" + String(ssid) + "\"}");
            }
        } else if (strcmp(cmd, "canvas_clear") == 0 || strcmp(cmd, "clear_canvas") == 0) {
            state.setMode(EffectMode::CUSTOM_PRESET);
            EffectManager::getInstance().setMode(EffectMode::CUSTOM_PRESET);
            EffectManager::getInstance().getCustomEffect().resetToBlankCanvas();
            sendStatusNotification("{\"event\":\"canvas_cleared\"}");
        }
    }

    if (doc["wifi_ssid"].is<const char*>()) {
        const char* ssid = doc["wifi_ssid"].as<const char*>();
        const char* pass = doc["wifi_pass"] | "";
        if (strlen(ssid) > 0) {
            LOG_I(TAG, "BLE: Received Wi-Fi credentials for SSID '%s'", ssid);
            WifiManager::getInstance().connectNewWifi(ssid, pass);
            sendStatusNotification("{\"event\":\"wifi_connecting\",\"ssid\":\"" + String(ssid) + "\"}");
        }
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

    // Safe delayed Connection Parameters update (Fix for Android GATT 133)
    // Wait 2500ms after connection so Android / iOS completely finishes GATT service & characteristic discovery
    if (deviceConnected && !connParamsUpdated && (millis() - connectedTime > 2500)) {
        connParamsUpdated = true;
        if (pServer) {
            // 16 (20ms) to 32 (40ms), latency 0, timeout 400 (4s) - optimal balance for Android and iOS
            pServer->updateConnParams(currentConnHandle, 16, 32, 0, 400);
            LOG_I(TAG, "GATT discovery safe window passed: connection parameters updated for handle %d", currentConnHandle);
        }
    }

    // Stale JSON buffer garbage collection
    if (!jsonRxBuffer.empty() && (millis() - lastJsonRxMillis > 1000)) {
        LOG_W(TAG, "Stale JSON buffer discarded on idle update");
        jsonRxBuffer.clear();
    }
}

