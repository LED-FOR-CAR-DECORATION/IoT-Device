#pragma once

#include <WiFi.h>

class WifiManager {
public:
    static WifiManager& getInstance();

    void init();
    void update();
    bool isConnected();
    String getIpAddress();

private:
    WifiManager();
    unsigned long lastReconnectAttempt;
    bool connected;
};
