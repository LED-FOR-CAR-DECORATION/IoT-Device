#include "CarSignalService.h"
#include "utils/Logger.h"

#define TAG "CAR_SIG"

CarSignalService& CarSignalService::getInstance() {
    static CarSignalService instance;
    return instance;
}

CarSignalService::CarSignalService()
    : currentState(TurnSignalState::IDLE) {}

void CarSignalService::init() {
    currentState = TurnSignalState::IDLE;
    LOG_I(TAG, "Car Signal Service Initialized [SIMULATOR MODE]");
    LOG_I(TAG, ">>> TEST KEYS: 'L'=Left, 'R'=Right, 'H'=Hazard, 'S'/'0'=Idle <<<");
}

void CarSignalService::update() {
    // Non-blocking Serial Input Simulator
    while (Serial.available() > 0) {
        char ch = static_cast<char>(Serial.read());

        // Skip newline and whitespace
        if (ch == '\r' || ch == '\n' || ch == ' ') {
            continue;
        }

        switch (ch) {
            case 'l':
            case 'L':
                setState(TurnSignalState::LEFT);
                break;

            case 'r':
            case 'R':
                setState(TurnSignalState::RIGHT);
                break;

            case 'h':
            case 'H':
                setState(TurnSignalState::HAZARD);
                break;

            case 's':
            case 'S':
            case '0':
                setState(TurnSignalState::IDLE);
                break;

            case '?':
                LOG_I(TAG, "Current Signal State: %s", getStateName());
                LOG_I(TAG, "Controls: [L]=Left Turn | [R]=Right Turn | [H]=Hazard | [S]=Stop/Idle");
                break;

            default:
                // Ignore unrecognized single characters
                break;
        }
    }
}

TurnSignalState CarSignalService::getState() const {
    return currentState;
}

void CarSignalService::setState(TurnSignalState state) {
    if (currentState != state) {
        currentState = state;
        LOG_I(TAG, "Turn Signal switched to: %s", getStateName());
    }
}

bool CarSignalService::isActive() const {
    return currentState != TurnSignalState::IDLE;
}

const char* CarSignalService::getStateName() const {
    switch (currentState) {
        case TurnSignalState::LEFT:   return "LEFT_TURN (<<<)";
        case TurnSignalState::RIGHT:  return "RIGHT_TURN (>>>)";
        case TurnSignalState::HAZARD: return "HAZARD (<<< >>>)";
        default:                      return "IDLE";
    }
}
