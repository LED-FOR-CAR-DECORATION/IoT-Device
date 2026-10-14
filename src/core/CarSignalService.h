#pragma once

#include "types.h"
#include <Arduino.h>

/**
 * @file CarSignalService.h
 * @brief Automotive signal manager with hardware abstraction & test simulator.
 * 
 * Manages turn signal states (Left, Right, Hazard, Idle).
 * In simulation mode, accepts Serial commands:
 *  - 'L' / 'l': Left Turn Signal
 *  - 'R' / 'r': Right Turn Signal
 *  - 'H' / 'h': Hazard Warning Signal
 *  - 'S' / 's' / '0': Turn Off (Idle)
 * 
 * Ready for future extension to CAN Bus (TWAI) or Optocoupler PC817.
 */
class CarSignalService {
public:
    static CarSignalService& getInstance();

    void init();
    void update();

    TurnSignalState getState() const;
    void setState(TurnSignalState state);
    bool isActive() const;
    const char* getStateName() const;

private:
    CarSignalService();
    volatile TurnSignalState currentState;
};
