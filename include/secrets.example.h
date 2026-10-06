#pragma once

/**
 * @file secrets.example.h
 * @brief Template for Wi-Fi and MQTT credentials.
 * Copy this file to "include/secrets.h" and fill in your actual credentials.
 * NOTE: DO NOT commit secrets.h to Git!
 */

// Wi-Fi Credentials
#define WIFI_SSID_DEFAULT       "YOUR_CAR_HOTSPOT_OR_HOME_WIFI"
#define WIFI_PASS_DEFAULT       "YOUR_WIFI_PASSWORD"

// MQTT Broker Credentials (Optional - for cloud integration)
#define MQTT_BROKER_HOST        "mqtt.yourdomain.com"
#define MQTT_BROKER_PORT        1883
#define MQTT_USERNAME           "device_user"
#define MQTT_PASSWORD           "device_secret_key"
#define MQTT_TOPIC_COMMAND      "car/led/command"
#define MQTT_TOPIC_STATUS       "car/led/status"

// BLE Pairing Passkey (Optional 6-digit PIN)
#define BLE_PIN_CODE            123456
