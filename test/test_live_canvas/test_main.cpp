#include <Arduino.h>
#include <unity.h>
#include "effects/EffectManager.h"
#include "utils/ColorUtils.h"

struct LiveCanvasTestProbe {
    static uint16_t pixel(CustomProjectionEffect& canvas, int x, int y) { return canvas.pixelBuffer[y * 64 + x]; }
    static bool dirty(CustomProjectionEffect& canvas) { return canvas.needsRedraw; }
};

void test_repeated_mode_selection_preserves_previous_strokes() {
    auto& effects = EffectManager::getInstance();
    auto& canvas = effects.getCustomEffect();
    TEST_ASSERT_TRUE(canvas.resetToBlankCanvas());
    const uint8_t red[] = {1,1,255,0,0};
    const uint8_t blue[] = {2,2,0,0,255};
    TEST_ASSERT_TRUE(canvas.drawLiveBatch(red,1));
    effects.setMode(EffectMode::CUSTOM_PRESET);
    TEST_ASSERT_TRUE(canvas.drawLiveBatch(blue,1));
    effects.setMode(EffectMode::CUSTOM_PRESET);
    TEST_ASSERT_EQUAL_HEX16(0xf800,LiveCanvasTestProbe::pixel(canvas,1,1));
    TEST_ASSERT_EQUAL_HEX16(0x001f,LiveCanvasTestProbe::pixel(canvas,2,2));
}

void test_batches_invalidate_renderer_and_reject_invalid_batch_atomically() {
    auto& canvas = EffectManager::getInstance().getCustomEffect();
    TEST_ASSERT_TRUE(canvas.resetToBlankCanvas());
    DeviceSettings settings;
    canvas.update(settings);
    TEST_ASSERT_FALSE(LiveCanvasTestProbe::dirty(canvas));
    const uint8_t green[] = {3,3,0,255,0};
    TEST_ASSERT_TRUE(canvas.drawLiveBatch(green,1));
    TEST_ASSERT_TRUE(LiveCanvasTestProbe::dirty(canvas));
    const uint8_t invalid[] = {4,4,255,255,255,64,4,255,0,0};
    TEST_ASSERT_FALSE(canvas.drawLiveBatch(invalid,2));
    TEST_ASSERT_EQUAL_HEX16(0,LiveCanvasTestProbe::pixel(canvas,4,4));
    TEST_ASSERT_EQUAL_HEX16(0x07e0,LiveCanvasTestProbe::pixel(canvas,3,3));
}

void setup() {
    delay(1500);
    UNITY_BEGIN();
    RUN_TEST(test_repeated_mode_selection_preserves_previous_strokes);
    RUN_TEST(test_batches_invalidate_renderer_and_reject_invalid_batch_atomically);
    UNITY_END();
}
void loop() {}
