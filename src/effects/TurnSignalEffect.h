#pragma once

#include "IEffect.h"
#include "types.h"
#include <Arduino.h>

/**
 * @file TurnSignalEffect.h
 * @brief Dynamic sequential turn signal & hazard animation for 64x32 LED Matrix.
 * 
 * Renders Audi/Lexus-style sequential flowing arrows in amber yellow:
 *  - LEFT: Flowing chevrons towards the left (<<<)
 *  - RIGHT: Flowing chevrons towards the right (>>>)
 *  - HAZARD: Outward dual-flowing chevrons or synchronized blinking (<<< >>>)
 */
class TurnSignalEffect : public IEffect {
public:
    TurnSignalEffect();
    virtual ~TurnSignalEffect() = default;

    void init() override;
    void update(const DeviceSettings& settings) override;
    const char* getName() const override { return "Sequential Turn Signal"; }
    EffectMode getMode() const override { return EffectMode::OFF; }

    void setSignalState(TurnSignalState state);

private:
    void renderRightTurn(unsigned long elapsedMs);
    void renderLeftTurn(unsigned long elapsedMs);
    void renderHazard(unsigned long elapsedMs);

    void drawChevron(int16_t xTip, bool pointRight, uint16_t color, uint8_t thickness = 3);

    TurnSignalState currentState;
    unsigned long cycleStartTime;
};
