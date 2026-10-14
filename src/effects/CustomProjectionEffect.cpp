#include "CustomProjectionEffect.h"
#include "drivers/LedDriver.h"
#include "storage/DefaultCarPattern.h"
#include "utils/Logger.h"

#define TAG "PROJECTION"

CustomProjectionEffect::CustomProjectionEffect()
    : currentFrameIndex(0), lastFrameTick(0), hasValidPattern(false), isLiveDrawingActive(false),
      lastRenderedFrameIndex(-1), needsRedraw(true) {
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

    // Reload saved pattern from flash if uninitialized or if returning from live drawing
    if (!hasValidPattern || isLiveDrawingActive) {
        if (!loadFromFlash(DEFAULT_PATTERN_PATH)) {
            LOG_I(TAG, "Loading default Car mascot animation from PROGMEM ROM");
            ProjectionHeader header;
            header.magic = PROJECTION_MAGIC;
            header.version = 2;
            header.frameCount = DEFAULT_CAR_FRAME_COUNT;
            header.width = DEFAULT_CAR_WIDTH;
            header.height = DEFAULT_CAR_HEIGHT;
            header.frameDelayMs = DEFAULT_CAR_DELAY_MS;
            header.colorFormat = 0;
            setFrames(header, reinterpret_cast<const uint8_t*>(DEFAULT_CAR_PIXELS), sizeof(DEFAULT_CAR_PIXELS));
        }
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
        isLiveDrawingActive = false;
        lastRenderedFrameIndex = -1;
        needsRedraw = true;
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

    // Auto-reset to pure blank black canvas if drawing starts while an image was displayed
    if (!isLiveDrawingActive) {
        resetToBlankCanvas();
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
        isLiveDrawingActive = true;
        lastRenderedFrameIndex = -1;
        needsRedraw = true;
        xSemaphoreGive(frameMutex);
    }
    LedDriver::getInstance().clear();
}

void CustomProjectionEffect::fillCanvas(uint16_t color565) {
    if (!isLiveDrawingActive) {
        resetToBlankCanvas();
    }

    if (xSemaphoreTake(frameMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
        if (!pixelBuffer.empty()) {
            std::fill(pixelBuffer.begin(), pixelBuffer.end(), color565);
        }
        needsRedraw = true;
        xSemaphoreGive(frameMutex);
    }
    LedDriver::getInstance().fill(color565);
}

void CustomProjectionEffect::drawLiveLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color565, bool isCommit) {
    if (!isLiveDrawingActive) {
        resetToBlankCanvas();
    }

    if (!isCommit) {
        // Real-time drag preview: re-blit current pixelBuffer, then overlay the preview line
        if (xSemaphoreTake(frameMutex, pdMS_TO_TICKS(20)) == pdTRUE) {
            if (!pixelBuffer.empty()) {
                LedDriver::getInstance().drawBitmap(0, 0, pixelBuffer.data(), 64, 32);
            }
            xSemaphoreGive(frameMutex);
        }
        LedDriver::getInstance().drawLine(x0, y0, x1, y1, color565);
    } else {
        // Commit line to pixelBuffer
        if (xSemaphoreTake(frameMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
            int16_t dx = abs(x1 - x0);
            int16_t dy = abs(y1 - y0);
            int16_t sx = (x0 < x1) ? 1 : -1;
            int16_t sy = (y0 < y1) ? 1 : -1;
            int16_t err = dx - dy;
            int16_t x = x0, y = y0;
            while (true) {
                if (x >= 0 && x < 64 && y >= 0 && y < 32) {
                    size_t idx = y * 64 + x;
                    if (idx < pixelBuffer.size()) {
                        pixelBuffer[idx] = color565;
                    }
                }
                if (x == x1 && y == y1) break;
                int16_t e2 = 2 * err;
                if (e2 > -dy) { err -= dy; x += sx; }
                if (e2 < dx) { err += dx; y += sy; }
            }
            needsRedraw = true;
            xSemaphoreGive(frameMutex);
        }
        LedDriver::getInstance().drawLine(x0, y0, x1, y1, color565);
    }
}

void CustomProjectionEffect::drawLiveRect(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color565, bool isCommit) {
    if (!isLiveDrawingActive) {
        resetToBlankCanvas();
    }

    int16_t rx = std::min(x0, x1);
    int16_t ry = std::min(y0, y1);
    int16_t rw = abs(x1 - x0) + 1;
    int16_t rh = abs(y1 - y0) + 1;

    if (!isCommit) {
        // Real-time drag preview: re-blit current pixelBuffer, then overlay preview rect
        if (xSemaphoreTake(frameMutex, pdMS_TO_TICKS(20)) == pdTRUE) {
            if (!pixelBuffer.empty()) {
                LedDriver::getInstance().drawBitmap(0, 0, pixelBuffer.data(), 64, 32);
            }
            xSemaphoreGive(frameMutex);
        }
        LedDriver::getInstance().drawRect(rx, ry, rw, rh, color565);
    } else {
        // Commit rectangle outline to pixelBuffer
        if (xSemaphoreTake(frameMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
            int16_t minX = std::min(x0, x1);
            int16_t maxX = std::max(x0, x1);
            int16_t minY = std::min(y0, y1);
            int16_t maxY = std::max(y0, y1);

            for (int16_t x = minX; x <= maxX; x++) {
                if (x >= 0 && x < 64) {
                    if (minY >= 0 && minY < 32) pixelBuffer[minY * 64 + x] = color565;
                    if (maxY >= 0 && maxY < 32) pixelBuffer[maxY * 64 + x] = color565;
                }
            }
            for (int16_t y = minY; y <= maxY; y++) {
                if (y >= 0 && y < 32) {
                    if (minX >= 0 && minX < 64) pixelBuffer[y * 64 + minX] = color565;
                    if (maxX >= 0 && maxX < 64) pixelBuffer[y * 64 + maxX] = color565;
                }
            }
            needsRedraw = true;
            xSemaphoreGive(frameMutex);
        }
        LedDriver::getInstance().drawRect(rx, ry, rw, rh, color565);
    }
}

void CustomProjectionEffect::floodFill(int16_t startX, int16_t startY, uint16_t newColor565) {
    if (startX < 0 || startX >= 64 || startY < 0 || startY >= 32) return;

    if (!isLiveDrawingActive) {
        resetToBlankCanvas();
    }

    if (xSemaphoreTake(frameMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        if (pixelBuffer.size() >= 2048) {
            uint16_t targetColor = pixelBuffer[startY * 64 + startX];
            if (targetColor != newColor565) {
                // BFS Queue with pre-allocated storage (max 2048 nodes)
                static uint16_t qx[2048];
                static uint16_t qy[2048];
                static bool visited[2048];
                memset(visited, 0, sizeof(visited));

                size_t head = 0, tail = 0;
                qx[tail] = startX;
                qy[tail] = startY;
                tail++;
                visited[startY * 64 + startX] = true;

                while (head < tail) {
                    int16_t cx = qx[head];
                    int16_t cy = qy[head];
                    head++;
                    pixelBuffer[cy * 64 + cx] = newColor565;

                    const int16_t dx[4] = {1, -1, 0, 0};
                    const int16_t dy[4] = {0, 0, 1, -1};
                    for (int i = 0; i < 4; i++) {
                        int16_t nx = cx + dx[i];
                        int16_t ny = cy + dy[i];
                        if (nx >= 0 && nx < 64 && ny >= 0 && ny < 32) {
                            size_t nIdx = ny * 64 + nx;
                            if (!visited[nIdx] && pixelBuffer[nIdx] == targetColor) {
                                visited[nIdx] = true;
                                qx[tail] = nx;
                                qy[tail] = ny;
                                tail++;
                            }
                        }
                    }
                }
                needsRedraw = true;
            }
        }
        xSemaphoreGive(frameMutex);
    }

    // Immediately blit updated pixelBuffer to LED matrix
    if (xSemaphoreTake(frameMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
        if (!pixelBuffer.empty()) {
            LedDriver::getInstance().drawBitmap(0, 0, pixelBuffer.data(), 64, 32);
        }
        xSemaphoreGive(frameMutex);
    }
}

void CustomProjectionEffect::clearFrames() {
    if (xSemaphoreTake(frameMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        pixelBuffer.clear();
        hasValidPattern = false;
        isLiveDrawingActive = false;
        currentFrameIndex = 0;
        lastRenderedFrameIndex = -1;
        needsRedraw = true;
        xSemaphoreGive(frameMutex);
    }
}

void CustomProjectionEffect::update(const DeviceSettings& settings) {
    LedDriver& led = LedDriver::getInstance();

    if (!hasValidPattern && !isLiveDrawingActive) {
        // Attempt lazy load once if uninitialized
        if (!loadFromFlash(DEFAULT_PATTERN_PATH)) {
            ProjectionHeader header;
            header.magic = PROJECTION_MAGIC;
            header.version = 2;
            header.frameCount = DEFAULT_CAR_FRAME_COUNT;
            header.width = DEFAULT_CAR_WIDTH;
            header.height = DEFAULT_CAR_HEIGHT;
            header.frameDelayMs = DEFAULT_CAR_DELAY_MS;
            header.colorFormat = 0;
            setFrames(header, reinterpret_cast<const uint8_t*>(DEFAULT_CAR_PIXELS), sizeof(DEFAULT_CAR_PIXELS));
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
                needsRedraw = true;
            }
        }

        // Only redraw to DMA buffer when frame actually changes or on explicit redraw request.
        // This completely eliminates continuous black-frame clearing and screen flickering!
        if (needsRedraw || currentFrameIndex != lastRenderedFrameIndex) {
            const size_t framePixels = currentHeader.width * currentHeader.height;
            const size_t frameOffset = currentFrameIndex * framePixels;

            if (frameOffset + framePixels <= pixelBuffer.size()) {
                const uint16_t* framePtr = pixelBuffer.data() + frameOffset;
                renderFrame(framePtr, currentHeader.width, currentHeader.height);
                lastRenderedFrameIndex = currentFrameIndex;
                needsRedraw = false;
            }
        }

        xSemaphoreGive(frameMutex);
    }
}

void CustomProjectionEffect::renderFrame(const uint16_t* frameData, uint16_t w, uint16_t h) {
    LedDriver& led = LedDriver::getInstance();

    // Only clear if the image doesn't cover the full matrix, avoiding black flashes
    if (w < led.getWidth() || h < led.getHeight()) {
        led.clear();
    }

    // Center image on the 64x32 matrix if it's smaller than the screen
    int16_t offsetX = (led.getWidth() - w) / 2;
    int16_t offsetY = (led.getHeight() - h) / 2;

    led.drawBitmap(offsetX, offsetY, frameData, w, h);
}
