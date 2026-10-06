#pragma once

#include "IEffect.h"

class StaticColorEffect : public IEffect {
public:
    void init() override;
    void update(const DeviceSettings& settings) override;
    const char* getName() const override { return "Static Color"; }
    EffectMode getMode() const override { return EffectMode::STATIC_COLOR; }
};
