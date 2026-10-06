#pragma once

#include "IEffect.h"
#include "StaticColorEffect.h"
#include "BreathingEffect.h"
#include "RainbowEffect.h"
#include "ChaseEffect.h"
#include "MusicSyncEffect.h"
#include <vector>
#include <memory>

class EffectManager {
public:
    static EffectManager& getInstance();

    void init();
    void update();
    void setMode(EffectMode mode);
    void nextMode();
    IEffect* getCurrentEffect();

private:
    EffectManager();

    std::vector<std::unique_ptr<IEffect>> effects;
    IEffect* activeEffect;
    StaticColorEffect staticEffect;
    BreathingEffect breathingEffect;
    RainbowEffect rainbowEffect;
    ChaseEffect chaseEffect;
    MusicSyncEffect musicEffect;

    unsigned long lastFrameTime;
};
