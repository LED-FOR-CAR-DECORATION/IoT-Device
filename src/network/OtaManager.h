#pragma once

#include <ArduinoOTA.h>

class OtaManager {
public:
    static OtaManager& getInstance();

    void init();
    void update();

private:
    OtaManager() = default;
};
