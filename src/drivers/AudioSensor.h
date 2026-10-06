#pragma once

#include <Arduino.h>

class AudioSensor {
public:
    static AudioSensor& getInstance();

    void init();
    uint16_t sampleAmplitude(uint8_t windowMs = 25);
    uint8_t getNormalizedLevel(uint8_t sensitivity = 60);

private:
    AudioSensor();
    uint16_t lastAmplitude;
};
