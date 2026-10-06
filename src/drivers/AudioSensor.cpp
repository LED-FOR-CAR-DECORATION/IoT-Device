#include "AudioSensor.h"
#include "config.h"
#include "utils/Logger.h"

#define TAG "AUDIO"

AudioSensor& AudioSensor::getInstance() {
    static AudioSensor instance;
    return instance;
}

AudioSensor::AudioSensor() : lastAmplitude(0) {}

void AudioSensor::init() {
    pinMode(PIN_MIC_ADC, INPUT);
    analogReadResolution(12); // 12-bit ADC (0 - 4095)
    LOG_I(TAG, "Audio Sensor initialized on GPIO %d", PIN_MIC_ADC);
}

uint16_t AudioSensor::sampleAmplitude(uint8_t windowMs) {
    unsigned long startMillis = millis();
    uint16_t minVal = 4095;
    uint16_t maxVal = 0;

    while (millis() - startMillis < windowMs) {
        uint16_t sample = analogRead(PIN_MIC_ADC);
        if (sample < minVal) minVal = sample;
        if (sample > maxVal) maxVal = sample;
    }

    uint16_t peakToPeak = (maxVal > minVal) ? (maxVal - minVal) : 0;
    lastAmplitude = peakToPeak;
    return peakToPeak;
}

uint8_t AudioSensor::getNormalizedLevel(uint8_t sensitivity) {
    uint16_t amp = sampleAmplitude(MIC_SAMPLE_WINDOW_MS);
    if (amp < MIC_NOISE_FLOOR) return 0;

    amp -= MIC_NOISE_FLOOR;
    // Scale by sensitivity factor (1 - 100)
    uint32_t scaled = (static_cast<uint32_t>(amp) * sensitivity) / 50;
    if (scaled > 255) scaled = 255;
    return static_cast<uint8_t>(scaled);
}
