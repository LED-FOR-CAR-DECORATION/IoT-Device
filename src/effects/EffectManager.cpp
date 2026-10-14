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
    : activeEffect(&customEffect), lastFrameTime(0) {}

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
        led.clear();
        led.show();
        return;
    }

    // Set brightness
    led.setBrightness(settings.brightness);

    // 1. Automotive Turn Signal Priority Override (Safety First)
    CarSignalService& carSignal = CarSignalService::getInstance();
    if (carSignal.isActive()) {
        turnSignalEffect.setSignalState(carSignal.getState());
        turnSignalEffect.update(settings);
        led.show();
        return;
    } else {
        turnSignalEffect.setSignalState(TurnSignalState::IDLE);
    }

    // 2. Render active standard animation
    if (activeEffect) {
        activeEffect->update(settings);
    }

    led.show();
}
