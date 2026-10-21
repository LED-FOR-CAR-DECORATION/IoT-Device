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
    bool drawLiveBatch(const uint8_t* points, uint8_t count);

    // Interactive shape drawing with live drag preview and commitment
    void drawLiveLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color565, bool isCommit);
    void drawLiveRect(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color565, bool isCommit);

    // Instant Flood Fill (Bucket Tool) and full-screen color fill
    void floodFill(int16_t startX, int16_t startY, uint16_t newColor565);
    void fillCanvas(uint16_t color565);

    // Reset to pure black blank canvas for interactive live drawing
    bool resetToBlankCanvas();
    void requestRedraw();

    // Clear active projection
    void clearFrames();

    // Query active projection info
    uint8_t getFrameCount() const { return currentHeader.frameCount; }
    uint16_t getWidth() const { return currentHeader.width; }
    uint16_t getHeight() const { return currentHeader.height; }

private:
    friend struct LiveCanvasTestProbe;
    ProjectionHeader currentHeader;
    std::vector<uint16_t> pixelBuffer; // RGB565 frames
    uint8_t currentFrameIndex;
    unsigned long lastFrameTick;
    SemaphoreHandle_t frameMutex;
    bool hasValidPattern;
    bool isLiveDrawingActive;
    int16_t lastRenderedFrameIndex;
    bool needsRedraw;

    void renderFrame(const uint16_t* frameData, uint16_t w, uint16_t h);
};
