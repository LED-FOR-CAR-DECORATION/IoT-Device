#pragma once

#include "IEffect.h"
#include "storage/StorageManager.h"
#include <vector>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

/**
 * @file CustomProjectionEffect.h
 * @brief Renders Pixel Art Characters and Animations onto HUB75 LED Matrix (Flow 2)
 */

class CustomProjectionEffect : public IEffect {
public:
    CustomProjectionEffect();
    ~CustomProjectionEffect() override;

    void init() override;
    void update(const DeviceSettings& settings) override;
    const char* getName() const override { return "CUSTOM_PROJECTION"; }
    EffectMode getMode() const override { return EffectMode::CUSTOM_PRESET; }

    // Load active pattern from SPIFFS flash
    bool loadFromFlash(const char* path = DEFAULT_PATTERN_PATH);

    // Set new animation frames directly into memory (from BLE/MQTT transfer)
    bool setFrames(const ProjectionHeader& header, const uint8_t* rawData, size_t dataSize);

    // Draw single pixel in real-time (for App interactive live pencil canvas)
    void drawLivePixel(int16_t x, int16_t y, uint16_t color565);

    // Reset to pure black blank canvas for interactive live drawing
    void resetToBlankCanvas();

    // Clear active projection
    void clearFrames();

    // Query active projection info
    uint8_t getFrameCount() const { return currentHeader.frameCount; }
    uint16_t getWidth() const { return currentHeader.width; }
    uint16_t getHeight() const { return currentHeader.height; }

private:
    ProjectionHeader currentHeader;
    std::vector<uint16_t> pixelBuffer; // RGB565 frames
    uint8_t currentFrameIndex;
    unsigned long lastFrameTick;
    SemaphoreHandle_t frameMutex;
    bool hasValidPattern;

    void renderFrame(const uint16_t* frameData, uint16_t w, uint16_t h);
};
