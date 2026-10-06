#pragma once

#include "IEffect.h"

class RainbowEffect : public IEffect {
public:
    void init() override;
    void update(const DeviceSettings& settings) override;
    const char* getName() const override { return "Rainbow Wave"; }
    EffectMode getMode() const override { return EffectMode::RAINBOW_WAVE; }

private:
    uint16_t hueOffset;
};
