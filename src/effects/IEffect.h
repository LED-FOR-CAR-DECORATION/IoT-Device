#pragma once

#include "types.h"
#include <Arduino.h>

class IEffect {
public:
    virtual ~IEffect() = default;

    virtual void init() = 0;
    virtual void update(const DeviceSettings& settings) = 0;
    virtual const char* getName() const = 0;
    virtual EffectMode getMode() const = 0;
};
