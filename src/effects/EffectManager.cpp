#include "EffectManager.h"
#include "drivers/LedDriver.h"
#include "core/StateManager.h"
#include "core/CarSignalService.h"
#include "utils/Logger.h"

#define TAG "EFFECTS"

EffectManager& EffectManager::getInstance() {
    static EffectManager instance;
    return instance;
}

EffectManager::EffectManager() 
    : activeEffect(&customEffect), lastFrameTime(0), displayOverridden(false) {}

void EffectManager::init() {
    staticEffect.init();
    breathingEffect.init();
    rainbowEffect.init();
    chaseEffect.init();
    customEffect.init();
    marqueeEffect.init();
    turnSignalEffect.init();

    setMode(StateManager::getInstance().getSettings().currentMode);
    LOG_I(TAG, "Effect Manager initialized");
}

void EffectManager::setMode(EffectMode mode) {
    // Every BLE drawing packet selects CUSTOM_PRESET. Reinitializing the already
    // active custom effect would reload flash and erase all previous live strokes.
    if ((mode == EffectMode::CUSTOM_PRESET || mode == EffectMode::WELCOME_CAR) && activeEffect == &customEffect) return;
    switch (mode) {
        case EffectMode::STATIC_COLOR:
            activeEffect = &staticEffect;
            break;
        case EffectMode::BREATHING:
            activeEffect = &breathingEffect;
            break;
        case EffectMode::RAINBOW_WAVE:
            activeEffect = &rainbowEffect;
            break;
        case EffectMode::CHASE_FLOW:
            activeEffect = &chaseEffect;
            break;
        case EffectMode::WELCOME_CAR:
        case EffectMode::CUSTOM_PRESET:
            activeEffect = &customEffect;
            break;
        case EffectMode::MARQUEE_TEXT:
            activeEffect = &marqueeEffect;
            break;
        default:
            activeEffect = &customEffect;
            break;
    }

    if (activeEffect) {
        activeEffect->init();
        LOG_I(TAG, "Switched effect to: %s", activeEffect->getName());
    }
}

void EffectManager::nextMode() {
    uint8_t modeInt = static_cast<uint8_t>(StateManager::getInstance().getSettings().currentMode);
    modeInt++;
    if (modeInt > static_cast<uint8_t>(EffectMode::MARQUEE_TEXT)) {
        modeInt = static_cast<uint8_t>(EffectMode::STATIC_COLOR);
    }

    EffectMode newMode = static_cast<EffectMode>(modeInt);
    StateManager::getInstance().setMode(newMode);
    setMode(newMode);
}

IEffect* EffectManager::getCurrentEffect() {
    return activeEffect;
}

void EffectManager::update() {
    DeviceSettings& settings = StateManager::getInstance().getSettings();
    LedDriver& led = LedDriver::getInstance();

    if (!settings.powerOn) {
        displayOverridden = true;
        led.clear();
        led.show();
        return;
    }

    // Set brightness
    led.setBrightness(settings.brightness);

    // One coherent snapshot for both cores. Priority: turn/hazard > brake > reverse > stopped.
    const auto vehicle = CarSignalService::getInstance().getSnapshot();
    const auto vehicleDisplay = VehicleCan::display(vehicle);
    if (vehicleDisplay != VehicleCan::Display::Normal) {
        displayOverridden = true;
        if (vehicle.turn != 0) {
            turnSignalEffect.setSignalState(static_cast<TurnSignalState>(vehicle.turn));
            turnSignalEffect.update(settings);
            // Keep braking/reversing visible even while arrows have priority.
            if (vehicle.fromCan && (vehicle.brake || vehicle.reverse)) {
                led.drawLine(0, MATRIX_HEIGHT - 1, MATRIX_WIDTH - 1, MATRIX_HEIGHT - 1,
                             vehicle.brake ? LedDriver::color565(255, 0, 0) : LedDriver::color565(255, 255, 255));
            }
        } else {
            turnSignalEffect.setSignalState(TurnSignalState::IDLE);
            led.clear();
            if (vehicleDisplay == VehicleCan::Display::Brake) {
                led.fill(255, 0, 0);
                led.drawText(17, 12, "BRAKE", LedDriver::color565(255, 255, 255));
            } else if (vehicleDisplay == VehicleCan::Display::Reverse) {
                led.fill(255, 255, 255);
                led.drawText(11, 12, "REVERSE", LedDriver::color565(0, 0, 0));
            } else {
                led.drawText(20, 12, "STOP", LedDriver::color565(255, 0, 0));
            }
        }
        led.show();
        return;
    } else {
        turnSignalEffect.setSignalState(TurnSignalState::IDLE);
    }

    if (displayOverridden) {
        customEffect.requestRedraw();
        displayOverridden = false;
    }

    // 2. Render active standard animation
    if (activeEffect) {
        activeEffect->update(settings);
    }

    led.show();
}
