#include "MqttManager.h"
#include "secrets.example.h"
#if __has_include("secrets.h")
#include "secrets.h"
#endif
#include "config.h"
#include "core/StateManager.h"
#include "effects/EffectManager.h"
#include "utils/Logger.h"
#include <ArduinoJson.h>

#define TAG "MQTT"
#define MQTT_RETRY_INTERVAL_MS 5000

MqttManager& MqttManager::getInstance() {
    static MqttManager instance;
    return instance;
}

MqttManager::MqttManager()
    : mqttClient(espClient), lastReconnectAttempt(0) {}

void MqttManager::init() {
    mqttClient.setServer(MQTT_BROKER_HOST, MQTT_BROKER_PORT);
    mqttClient.setCallback(MqttManager::callback);
    LOG_I(TAG, "MQTT configured for broker %s:%d", MQTT_BROKER_HOST, MQTT_BROKER_PORT);
}

void MqttManager::reconnect() {
    if (WiFi.status() != WL_CONNECTED) return;

    if (millis() - lastReconnectAttempt > MQTT_RETRY_INTERVAL_MS) {
        lastReconnectAttempt = millis();
        LOG_I(TAG, "Connecting to MQTT Broker...");

        String clientId = String(DEVICE_NAME) + "-" + String(WiFi.macAddress());
        if (mqttClient.connect(clientId.c_str(), MQTT_USERNAME, MQTT_PASSWORD)) {
            LOG_I(TAG, "MQTT Connected successfully!");
            mqttClient.subscribe(MQTT_TOPIC_COMMAND);
        } else {
            LOG_W(TAG, "MQTT connection failed, rc=%d", mqttClient.state());
        }
    }
}

void MqttManager::update() {
    if (!mqttClient.connected()) {
        reconnect();
    } else {
        mqttClient.loop();
    }
}

void MqttManager::publishStatus(const char* payload) {
    if (mqttClient.connected()) {
        mqttClient.publish(MQTT_TOPIC_STATUS, payload);
    }
}

void MqttManager::callback(char* topic, byte* payload, unsigned int length) {
    LOG_I(TAG, "Message arrived on topic [%s]", topic);

    char message[length + 1];
    memcpy(message, payload, length);
    message[length] = '\0';

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, message);
    if (error) {
        LOG_W(TAG, "MQTT JSON parse failed: %s", error.c_str());
        return;
    }

    StateManager& state = StateManager::getInstance();
    if (doc["power"].is<bool>()) {
        state.setPower(doc["power"].as<bool>());
    }
    if (doc["brightness"].is<uint8_t>() || doc["brightness"].is<int>()) {
        state.setBrightness(doc["brightness"].as<uint8_t>());
    }
    if (doc["speed"].is<uint8_t>() || doc["speed"].is<int>()) {
        state.setSpeed(doc["speed"].as<uint8_t>());
    }
    if (doc["mode"].is<uint8_t>() || doc["mode"].is<int>()) {
        uint8_t m = doc["mode"].as<uint8_t>();
        state.setMode(static_cast<EffectMode>(m));
        EffectManager::getInstance().setMode(static_cast<EffectMode>(m));
    }
    if (doc["color"].is<JsonObject>()) {
        uint8_t r = doc["color"]["r"] | 255;
        uint8_t g = doc["color"]["g"] | 255;
        uint8_t b = doc["color"]["b"] | 255;
        state.setPrimaryColor(r, g, b);
    }
}
