#pragma once

#include "IEffect.h"

class ChaseEffect : public IEffect {
public:
    void init() override;
    void update(const DeviceSettings& settings) override;
    const char* getName() const override { return "Chase Flow"; }
    EffectMode getMode() const override { return EffectMode::CHASE_FLOW; }

private:
    uint16_t headPos;
};
