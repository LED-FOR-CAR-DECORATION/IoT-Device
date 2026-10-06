#include "PowerManager.h"
#include "config.h"
#include "utils/Logger.h"

#define TAG "POWER"

PowerManager& PowerManager::getInstance() {
    static PowerManager instance;
    return instance;
}

PowerManager::PowerManager() : lastAccState(true), lastDoorState(false) {}

void PowerManager::init() {
    pinMode(PIN_CAR_ACC_DETECT, INPUT_PULLDOWN);
    pinMode(PIN_CAR_DOOR_TRIGGER, INPUT_PULLDOWN);
    LOG_I(TAG, "Power and Automotive trigger inputs initialized");
}

void PowerManager::update() {
    bool currentAcc = digitalRead(PIN_CAR_ACC_DETECT);
    if (currentAcc != lastAccState) {
        lastAccState = currentAcc;
        LOG_I(TAG, "ACC Ignition state changed: %s", currentAcc ? "ON" : "OFF");
    }

    bool currentDoor = digitalRead(PIN_CAR_DOOR_TRIGGER);
    if (currentDoor != lastDoorState) {
        lastDoorState = currentDoor;
        LOG_I(TAG, "Door trigger state changed: %s", currentDoor ? "OPEN" : "CLOSED");
    }
}

bool PowerManager::isIgnitionOn() {
    return lastAccState;
}

bool PowerManager::isDoorOpen() {
    return lastDoorState;
}
