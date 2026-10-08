#include "StorageManager.h"
#include "DefaultCarPattern.h"
#include "utils/Logger.h"

#define TAG "STORAGE"

StorageManager& StorageManager::getInstance() {
    static StorageManager instance;
    return instance;
}

StorageManager::StorageManager() : fsMounted(false) {}

bool StorageManager::init() {
    if (!SPIFFS.begin(true)) {
        LOG_E(TAG, "Failed to mount SPIFFS filesystem!");
        fsMounted = false;
        return false;
    }

    fsMounted = true;
    LOG_I(TAG, "SPIFFS mounted successfully. Total: %u KB, Used: %u KB",
          SPIFFS.totalBytes() / 1024, SPIFFS.usedBytes() / 1024);

    generateDefaultCarPatternIfMissing();
    return true;
}

bool StorageManager::savePattern(const char* path, const ProjectionHeader& header, const uint8_t* rawData, size_t dataSize) {
    if (!fsMounted) {
        LOG_E(TAG, "Cannot save pattern: SPIFFS not mounted");
        return false;
    }

    File file = SPIFFS.open(path, FILE_WRITE);
    if (!file) {
        LOG_E(TAG, "Failed to open file '%s' for writing", path);
        return false;
    }

    // 1. Write Header
    size_t writtenHeader = file.write(reinterpret_cast<const uint8_t*>(&header), sizeof(ProjectionHeader));
    if (writtenHeader != sizeof(ProjectionHeader)) {
        LOG_E(TAG, "Failed to write header to '%s'", path);
        file.close();
        return false;
    }

    // 2. Write Payload
    size_t writtenData = file.write(rawData, dataSize);
    file.close();

    if (writtenData != dataSize) {
        LOG_E(TAG, "Failed to write all data bytes to '%s' (%u of %u)", path, (unsigned int)writtenData, (unsigned int)dataSize);
        return false;
    }

    LOG_I(TAG, "Pattern saved to '%s': %dx%d, %d frames, %u bytes",
          path, header.width, header.height, header.frameCount, (unsigned int)(sizeof(ProjectionHeader) + dataSize));
    return true;
}

bool StorageManager::loadPattern(const char* path, ProjectionHeader& outHeader, std::vector<uint8_t>& outData) {
    if (!fsMounted) {
        LOG_E(TAG, "Cannot load pattern: SPIFFS not mounted");
        return false;
    }

    if (!SPIFFS.exists(path)) {
        LOG_W(TAG, "Pattern file '%s' does not exist", path);
        return false;
    }

    File file = SPIFFS.open(path, FILE_READ);
    if (!file) {
        LOG_E(TAG, "Failed to open file '%s' for reading", path);
        return false;
    }

    // 1. Read Header
    size_t readHeader = file.read(reinterpret_cast<uint8_t*>(&outHeader), sizeof(ProjectionHeader));
    if (readHeader != sizeof(ProjectionHeader) || outHeader.magic != PROJECTION_MAGIC) {
        LOG_E(TAG, "Invalid projection file header in '%s' (magic: 0x%08X)", path, outHeader.magic);
        file.close();
        return false;
    }

    // 2. Read Payload
    size_t remainingBytes = file.size() - sizeof(ProjectionHeader);
    outData.resize(remainingBytes);

    size_t bytesRead = file.read(outData.data(), remainingBytes);
    file.close();

    if (bytesRead != remainingBytes) {
        LOG_W(TAG, "Read incomplete data from '%s' (%u of %u)", path, (unsigned int)bytesRead, (unsigned int)remainingBytes);
    }

    LOG_I(TAG, "Loaded pattern from '%s': %dx%d, %d frames (%u bytes)",
          path, outHeader.width, outHeader.height, outHeader.frameCount, (unsigned int)outData.size());
    return true;
}

bool StorageManager::hasPattern(const char* path) {
    return fsMounted && SPIFFS.exists(path);
}

bool StorageManager::deletePattern(const char* path) {
    if (!fsMounted) return false;
    if (SPIFFS.exists(path)) {
        return SPIFFS.remove(path);
    }
    return false;
}

size_t StorageManager::getFileSize(const char* path) {
    if (!fsMounted || !SPIFFS.exists(path)) return 0;
    File f = SPIFFS.open(path, FILE_READ);
    if (!f) return 0;
    size_t sz = f.size();
    f.close();
    return sz;
}

void StorageManager::generateDefaultCarPatternIfMissing() {
    ProjectionHeader header;
    header.magic = PROJECTION_MAGIC;
    header.version = 2; // Version 2: Blue Sports Car Pixel Art Mascot
    header.frameCount = DEFAULT_CAR_FRAME_COUNT;
    header.width = DEFAULT_CAR_WIDTH;
    header.height = DEFAULT_CAR_HEIGHT;
    header.frameDelayMs = DEFAULT_CAR_DELAY_MS;
    header.colorFormat = 0; // RGB565

    if (hasPattern(DEFAULT_PATTERN_PATH)) {
        ProjectionHeader existingHeader;
        std::vector<uint8_t> dummy;
        if (loadPattern(DEFAULT_PATTERN_PATH, existingHeader, dummy)) {
            if (existingHeader.version >= 2) {
                LOG_I(TAG, "Default car pattern (v%d) already loaded in SPIFFS", existingHeader.version);
                return;
            }
        }
    }

    LOG_I(TAG, "Deploying Blue Sports Car Pixel Art mascot to '%s' (v2)...", DEFAULT_PATTERN_PATH);
    const size_t dataSize = sizeof(DEFAULT_CAR_PIXELS);
    savePattern(DEFAULT_PATTERN_PATH, header, reinterpret_cast<const uint8_t*>(DEFAULT_CAR_PIXELS), dataSize);
}
