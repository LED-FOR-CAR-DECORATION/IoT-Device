#pragma once

/**
 * @file pin_definitions.h
 * @brief Pin configuration for ESP32 Car LED Decoration Controller
 */

// HUB75 RGB LED Matrix Pins (DMA I2S Controller for ESP32)
#define PIN_HUB75_R1            25
#define PIN_HUB75_G1            26
#define PIN_HUB75_B1            27
#define PIN_HUB75_R2            14
#define PIN_HUB75_G2            12
#define PIN_HUB75_B2            13
#define PIN_HUB75_A             23
#define PIN_HUB75_B             19
#define PIN_HUB75_C             5
#define PIN_HUB75_D             17
#define PIN_HUB75_E             -1      // Not used for 64x32 panels
#define PIN_HUB75_LAT           4
#define PIN_HUB75_OE            15
#define PIN_HUB75_CLK           16

// Emergency Stop (E-Stop) Button (Pin NO -> GPIO 32, Pin C -> GND)
#define PIN_E_STOP              32

// On-board BOOT button, active LOW. Hold while firmware is running.
#define PIN_FACTORY_RESET       0

// SN65HVD230 TWAI interface; GPIO4/5 are already used by HUB75.
#define PIN_CAN_TX              21
#define PIN_CAN_RX              22
