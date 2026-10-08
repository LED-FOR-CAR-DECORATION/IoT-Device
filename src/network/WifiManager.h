#pragma once

#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>

class WifiManager {
public:
    static WifiManager& getInstance();

    void init();
    void update();
    bool isConnected();
    String getIpAddress();
    String getSsid() const { return currentSsid; }

    void connectNewWifi(const String& ssid, const String& password);

private:
    WifiManager();
    unsigned long lastReconnectAttempt;
    bool connected;
    String currentSsid;
    String currentPass;
    WebServer server;
    bool webServerStarted;

    void loadFromNVS();
    void saveToNVS(const String& ssid, const String& password);
    void startWebServer();
    void handleRoot();
    void handleApiCommand();
    void handleApiUpload();
    void handleApiWifi();
    void handleApiStatus();
};
