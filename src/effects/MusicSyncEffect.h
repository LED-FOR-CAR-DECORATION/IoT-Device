#pragma once

#include "IEffect.h"

class MusicSyncEffect : public IEffect {
public:
    void init() override;
    void update(const DeviceSettings& settings) override;
    const char* getName() const override { return "Music Reactive"; }
    EffectMode getMode() const override { return EffectMode::MUSIC_REACTIVE; }

private:
    uint16_t peakHold;
    unsigned long lastDecay;
};
