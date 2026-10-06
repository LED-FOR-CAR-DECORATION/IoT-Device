#pragma once

/**
 * @file pin_definitions.h
 * @brief Pin configuration for ESP32 Car LED Decoration Controller
 */

// LED Strips Data Pins
#define PIN_LED_DATA_MAIN       18      // Main interior ambient strip (WS2812B / SK6812)
#define PIN_LED_DATA_ZONE2      19      // Optional Zone 2 (e.g. Footwells / Door panels)

// Audio Sensor Pin (Music Reactive Mode)
#define PIN_MIC_ADC             34      // Analog Microphone (MAX4466 / MAX9814) input pin

// Hardware Control Pins
#define PIN_BUTTON_MODE         0       // Boot/Function button to toggle modes manually
#define PIN_STATUS_LED          2       // Onboard status indicator LED

// Power Management / Automotive Trigger Inputs (Optional)
#define PIN_CAR_ACC_DETECT      35      // ACC (Ignition) detection via optocoupler / divider
#define PIN_CAR_DOOR_TRIGGER    32      // Door open trigger (Welcome light effect)
