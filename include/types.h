#pragma once

#include <Arduino.h>

/**
 * @file types.h
 * @brief Common data structures, enums, and types for Car LED System
 */

// Available Lighting Effect Modes
enum class EffectMode : uint8_t {
    OFF = 0,
    STATIC_COLOR,
    BREATHING,
    RAINBOW_WAVE,
    CHASE_FLOW,
    WELCOME_CAR,
    CUSTOM_PRESET,
    MARQUEE_TEXT
};

// RGB Color structure
struct RGBColor {
    uint8_t r;
    uint8_t g;
    uint8_t b;

    RGBColor(uint8_t red = 255, uint8_t green = 255, uint8_t blue = 255)
        : r(red), g(green), b(blue) {}
};

// Runtime Device Settings (Saved to NVS)
struct DeviceSettings {
    bool powerOn;
    uint8_t brightness;         // 0 - 255
    uint8_t speed;              // 1 - 100
    EffectMode currentMode;
    RGBColor primaryColor;
    RGBColor secondaryColor;

    DeviceSettings()
        : powerOn(true),
          brightness(180),
          speed(50),
          currentMode(EffectMode::WELCOME_CAR),
          primaryColor(0, 150, 255),    // Default Ice Blue
          secondaryColor(255, 0, 120) {} // Neon Pink
};

// System connection states
enum class ConnectionState : uint8_t {
    DISCONNECTED,
    CONNECTING,
    CONNECTED,
    ERROR_STATE
};
