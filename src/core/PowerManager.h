#pragma once

#include <Arduino.h>

class PowerManager {
public:
    static PowerManager& getInstance();

    void init();
    void update();
    bool isIgnitionOn();
    bool isDoorOpen();

private:
    PowerManager();
    bool lastAccState;
    bool lastDoorState;
};
