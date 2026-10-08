#include "CustomProjectionEffect.h"
#include "drivers/LedDriver.h"
#include "utils/Logger.h"

#define TAG "PROJECTION"

CustomProjectionEffect::CustomProjectionEffect()
    : currentFrameIndex(0), lastFrameTick(0), hasValidPattern(false) {
    frameMutex = xSemaphoreCreateMutex();
}

CustomProjectionEffect::~CustomProjectionEffect() {
    if (frameMutex != nullptr) {
        vSemaphoreDelete(frameMutex);
    }
}

void CustomProjectionEffect::init() {
    currentFrameIndex = 0;
    lastFrameTick = millis();

    if (!loadFromFlash(DEFAULT_PATTERN_PATH)) {
        LOG_W(TAG, "No default pattern found in SPIFFS flash");
    }
}

bool CustomProjectionEffect::loadFromFlash(const char* path) {
    StorageManager& storage = StorageManager::getInstance();
    if (!storage.isAvailable() && !storage.init()) {
        LOG_E(TAG, "StorageManager not available");
        return false;
    }

    ProjectionHeader header;
    std::vector<uint8_t> rawBytes;

    if (!storage.loadPattern(path, header, rawBytes)) {
        return false;
    }

    return setFrames(header, rawBytes.data(), rawBytes.size());
}

bool CustomProjectionEffect::setFrames(const ProjectionHeader& header, const uint8_t* rawData, size_t dataSize) {
    if (rawData == nullptr || dataSize == 0 || header.frameCount == 0) {
        LOG_E(TAG, "Invalid frame data parameters");
        return false;
    }

    const size_t expectedBytes = header.frameCount * header.width * header.height * sizeof(uint16_t);
    if (dataSize < expectedBytes) {
        LOG_W(TAG, "Data size (%u) smaller than expected (%u)", (unsigned int)dataSize, (unsigned int)expectedBytes);
    }

    if (xSemaphoreTake(frameMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        currentHeader = header;
        const size_t pixelCount = header.frameCount * header.width * header.height;
        pixelBuffer.resize(pixelCount);

        const uint16_t* srcPixels = reinterpret_cast<const uint16_t*>(rawData);
        const size_t copyPixels = std::min(pixelCount, dataSize / sizeof(uint16_t));
        memcpy(pixelBuffer.data(), srcPixels, copyPixels * sizeof(uint16_t));

        currentFrameIndex = 0;
        lastFrameTick = millis();
        hasValidPattern = true;
        xSemaphoreGive(frameMutex);

        LOG_I(TAG, "Loaded %d projection frame(s) [%dx%d, delay: %dms]",
              header.frameCount, header.width, header.height, header.frameDelayMs);
        return true;
    }

    LOG_E(TAG, "Failed to acquire frame mutex");
    return false;
}

void CustomProjectionEffect::drawLivePixel(int16_t x, int16_t y, uint16_t color565) {
    if (x < 0 || x >= currentHeader.width || y < 0 || y >= currentHeader.height) {
        return;
    }

    if (xSemaphoreTake(frameMutex, pdMS_TO_TICKS(20)) == pdTRUE) {
        if (hasValidPattern && !pixelBuffer.empty()) {
            size_t idx = (currentFrameIndex * (currentHeader.width * currentHeader.height)) + (y * currentHeader.width + x);
            if (idx < pixelBuffer.size()) {
                pixelBuffer[idx] = color565;
            }
        }
        xSemaphoreGive(frameMutex);
    }

    // Direct draw to matrix for immediate 0-latency visual feedback
    int16_t offsetX = (LedDriver::getInstance().getWidth() - currentHeader.width) / 2;
    int16_t offsetY = (LedDriver::getInstance().getHeight() - currentHeader.height) / 2;
    LedDriver::getInstance().drawPixel(x + offsetX, y + offsetY, color565);
}

void CustomProjectionEffect::resetToBlankCanvas() {
    if (xSemaphoreTake(frameMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        currentHeader.magic = PROJECTION_MAGIC;
        currentHeader.version = 1;
        currentHeader.frameCount = 1;
        currentHeader.width = 64;
        currentHeader.height = 32;
        currentHeader.frameDelayMs = 1000;
        currentHeader.colorFormat = 0;

        pixelBuffer.assign(64 * 32, 0x0000); // 2048 black pixels
        currentFrameIndex = 0;
        hasValidPattern = true;
        xSemaphoreGive(frameMutex);
    }
    LedDriver::getInstance().clear();
}

void CustomProjectionEffect::clearFrames() {
    if (xSemaphoreTake(frameMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        pixelBuffer.clear();
        hasValidPattern = false;
        currentFrameIndex = 0;
        xSemaphoreGive(frameMutex);
    }
}

void CustomProjectionEffect::update(const DeviceSettings& settings) {
    LedDriver& led = LedDriver::getInstance();

    if (!hasValidPattern) {
        // Attempt lazy load once if uninitialized
        if (!loadFromFlash(DEFAULT_PATTERN_PATH)) {
            led.clear();
            led.drawText(4, 12, "NO PATTERN", LedDriver::color565(255, 50, 50), 1);
            return;
        }
    }

    if (xSemaphoreTake(frameMutex, pdMS_TO_TICKS(10)) == pdTRUE) {
        if (currentHeader.frameCount > 1) {
            // Speed scaling: speed 50 = 1.0x, speed 100 = 2.0x, speed 1 = 0.1x
            uint8_t spd = settings.speed > 0 ? settings.speed : 50;
            uint32_t delayMs = (currentHeader.frameDelayMs * 50) / spd;
            if (delayMs < 20) delayMs = 20;

            if (millis() - lastFrameTick >= delayMs) {
                currentFrameIndex = (currentFrameIndex + 1) % currentHeader.frameCount;
                lastFrameTick = millis();
            }
        }

        const size_t framePixels = currentHeader.width * currentHeader.height;
        const size_t frameOffset = currentFrameIndex * framePixels;

        if (frameOffset + framePixels <= pixelBuffer.size()) {
            const uint16_t* framePtr = pixelBuffer.data() + frameOffset;
            renderFrame(framePtr, currentHeader.width, currentHeader.height);
        }

        xSemaphoreGive(frameMutex);
    }
}

void CustomProjectionEffect::renderFrame(const uint16_t* frameData, uint16_t w, uint16_t h) {
    LedDriver& led = LedDriver::getInstance();
    led.clear();

    // Center image on the 64x32 matrix if it's smaller than the screen
    int16_t offsetX = (led.getWidth() - w) / 2;
    int16_t offsetY = (led.getHeight() - h) / 2;

    led.drawBitmap(offsetX, offsetY, frameData, w, h);
}
