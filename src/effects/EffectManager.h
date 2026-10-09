#pragma once

#include "IEffect.h"
#include "StaticColorEffect.h"
#include "BreathingEffect.h"
#include "RainbowEffect.h"
#include "ChaseEffect.h"
#include "CustomProjectionEffect.h"
#include "MarqueeTextEffect.h"
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
    CustomProjectionEffect& getCustomEffect() { return customEffect; }
    MarqueeTextEffect& getMarqueeEffect() { return marqueeEffect; }

private:
    EffectManager();

    std::vector<std::unique_ptr<IEffect>> effects;
    IEffect* activeEffect;
    StaticColorEffect staticEffect;
    BreathingEffect breathingEffect;
    RainbowEffect rainbowEffect;
    ChaseEffect chaseEffect;
    CustomProjectionEffect customEffect;
    MarqueeTextEffect marqueeEffect;

    unsigned long lastFrameTime;
};
