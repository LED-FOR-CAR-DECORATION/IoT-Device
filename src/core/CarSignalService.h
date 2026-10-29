#pragma once
#include "types.h"
#include "VehicleCanProtocol.h"
#include <Arduino.h>

class CarSignalService {
public:
    static CarSignalService& getInstance();
    void init();
    void update();
    VehicleCan::Snapshot getSnapshot() const;
    TurnSignalState getState() const;
    void setState(TurnSignalState state);
    bool isActive() const;
    const char* getStateName() const;
private:
    CarSignalService() = default;
    VehicleCan::Snapshot snapshot;
    mutable portMUX_TYPE stateMux = portMUX_INITIALIZER_UNLOCKED;
    bool canReady = false;
    bool staleReported = false;
    uint32_t lastLog = 0;
};
