#pragma once

#include <Arduino.h>
#include "pin_definitions.h"
#include "types.h"

/**
 * @file config.h
 * @brief Global configuration constants for Car LED System
 */

// Device Identity
#define DEVICE_NAME             "CarLED-Ambient"
#ifndef FIRMWARE_VERSION
#define FIRMWARE_VERSION        "1.1.0"
#endif
#define HARDWARE_REV            "ESP32-V1"

// Matrix Hardware Specifications
#define MATRIX_WIDTH            64      // Width in pixels
#define MATRIX_HEIGHT           32      // Height in pixels
#define MATRIX_CHAIN            1       // Number of chained panels
#define DEFAULT_BRIGHTNESS      180     // Default brightness (0-255)


// Bluetooth Low Energy (BLE) UUIDs
#define BLE_SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define BLE_CHAR_CONTROL_UUID   "beb5483e-36e1-4688-b7f5-ea07361b26a8"
#define BLE_CHAR_STATUS_UUID    "1c95d5e3-d8f7-413a-bf3d-7a2e5d7be87e"

// Task Configuration (FreeRTOS)
#define CORE_LED_RENDER         1       // Core 1 dedicated to LED animation
#define CORE_NETWORK            0       // Core 0 dedicated to BLE / WiFi / Sensors
#define STACK_SIZE_LED_TASK     4096
#define STACK_SIZE_NET_TASK     4096
