#pragma once

#include <WiFi.h>
#include <WiFiClient.h>
#include <PubSubClient.h>

class MqttManager {
public:
    static MqttManager& getInstance();

    void init();
    void update();
    void publishStatus(const char* payload);

private:
    MqttManager();
    WiFiClient espClient;
    PubSubClient mqttClient;
    unsigned long lastReconnectAttempt;

    void reconnect();
    static void callback(char* topic, byte* payload, unsigned int length);
};
