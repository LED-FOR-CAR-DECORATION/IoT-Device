#include "WifiManager.h"
#include "secrets.example.h"
#if __has_include("secrets.h")
#include "secrets.h"
#endif
#include "utils/Logger.h"

#define TAG "WIFI"
#define RECONNECT_INTERVAL_MS 10000

WifiManager& WifiManager::getInstance() {
    static WifiManager instance;
    return instance;
}

WifiManager::WifiManager() : lastReconnectAttempt(0), connected(false) {}

void WifiManager::init() {
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID_DEFAULT, WIFI_PASS_DEFAULT);
    LOG_I(TAG, "Connecting to Wi-Fi SSID: %s...", WIFI_SSID_DEFAULT);
}

void WifiManager::update() {
    if (WiFi.status() == WL_CONNECTED) {
        if (!connected) {
            connected = true;
            LOG_I(TAG, "Wi-Fi Connected! IP: %s", WiFi.localIP().toString().c_str());
        }
    } else {
        if (connected) {
            connected = false;
            LOG_W(TAG, "Wi-Fi Disconnected!");
        }

        if (millis() - lastReconnectAttempt > RECONNECT_INTERVAL_MS) {
            lastReconnectAttempt = millis();
            LOG_I(TAG, "Attempting Wi-Fi reconnection...");
            WiFi.reconnect();
        }
    }
}

bool WifiManager::isConnected() {
    return WiFi.status() == WL_CONNECTED;
}

String WifiManager::getIpAddress() {
    return WiFi.localIP().toString();
}
