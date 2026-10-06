#pragma once

#include "IEffect.h"

class BreathingEffect : public IEffect {
public:
    void init() override;
    void update(const DeviceSettings& settings) override;
    const char* getName() const override { return "Breathing"; }
    EffectMode getMode() const override { return EffectMode::BREATHING; }

private:
    float phase;
};
