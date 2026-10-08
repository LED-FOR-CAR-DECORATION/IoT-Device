#pragma once

#include <Arduino.h>
#include <FS.h>
#include <SPIFFS.h>
#include <vector>

/**
 * @file StorageManager.h
 * @brief Manages SPIFFS Flash storage for Pixel Art Character Projections (Flow 2)
 */

#define PROJECTION_MAGIC 0x50584152 // "PXAR" in ASCII (Pixel Art)
#define DEFAULT_PATTERN_PATH "/active_pattern.bin"

// Header for stored projection files
struct ProjectionHeader {
    uint32_t magic;          // 0x50584152 ("PXAR")
    uint8_t version;         // Protocol version (1)
    uint8_t frameCount;      // Number of frames (1 for static, >1 for animated)
    uint16_t width;          // Pixel width (typically 64)
    uint16_t height;         // Pixel height (typically 32)
    uint16_t frameDelayMs;   // Delay between animation frames in ms
    uint8_t colorFormat;     // 0 = RGB565 (2 bytes per pixel)
    uint8_t reserved[5];     // Padding

    ProjectionHeader()
        : magic(PROJECTION_MAGIC), version(1), frameCount(1),
          width(64), height(32), frameDelayMs(200), colorFormat(0) {
        memset(reserved, 0, sizeof(reserved));
    }
};

class StorageManager {
public:
    static StorageManager& getInstance();

    bool init();
    bool isAvailable() const { return fsMounted; }

    // Save and load complete projection pattern
    bool savePattern(const char* path, const ProjectionHeader& header, const uint8_t* rawData, size_t dataSize);
    bool loadPattern(const char* path, ProjectionHeader& outHeader, std::vector<uint8_t>& outData);

    // Helper checks
    bool hasPattern(const char* path = DEFAULT_PATTERN_PATH);
    bool deletePattern(const char* path = DEFAULT_PATTERN_PATH);
    size_t getFileSize(const char* path);

    // Built-in initial pattern generation (Blue Sports Car Pixel Art Mascot)
    void generateDefaultCarPatternIfMissing();

private:
    StorageManager();
    bool fsMounted;
};
