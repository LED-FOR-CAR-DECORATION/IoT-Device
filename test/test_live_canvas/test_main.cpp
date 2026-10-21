#include <Arduino.h>
#include <unity.h>
#include "core/FactoryResetButton.h"
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

void test_factory_reset_requires_continuous_five_second_hold() {
    FactoryResetButton button;
    TEST_ASSERT_FALSE(button.update(false, 0));
    TEST_ASSERT_FALSE(button.update(true, 100));
    TEST_ASSERT_FALSE(button.update(true, 130));
    TEST_ASSERT_FALSE(button.update(true, 5129));
    TEST_ASSERT_TRUE(button.update(true, 5130));
    TEST_ASSERT_FALSE(button.update(true, 9000));
}

void test_factory_reset_short_press_release_and_bounce_cancel_hold() {
    FactoryResetButton button;
    button.update(false, 0);
    button.update(true, 100);
    TEST_ASSERT_FALSE(button.update(false, 110));
    TEST_ASSERT_FALSE(button.update(false, 6000));
    button.update(true, 6100);
    button.update(true, 6130);
    TEST_ASSERT_FALSE(button.update(false, 10000));
    button.update(false, 10030);
    button.update(true, 11000);
    button.update(true, 11030);
    TEST_ASSERT_FALSE(button.update(true, 16029));
    TEST_ASSERT_TRUE(button.update(true, 16030));
}

void test_factory_reset_survives_millis_wrap_and_requires_release_after_reboot() {
    FactoryResetButton button;
    TEST_ASSERT_FALSE(button.update(true, 0));
    TEST_ASSERT_FALSE(button.update(true, 30));
    TEST_ASSERT_FALSE(button.update(true, 6000));
    button.update(false, 6100);
    button.update(false, 6130);
    button.update(true, UINT32_MAX - 100);
    button.update(true, UINT32_MAX - 70);
    TEST_ASSERT_FALSE(button.update(true, 4928));
    TEST_ASSERT_TRUE(button.update(true, 4929));
}

void setup() {
    delay(1500);
    UNITY_BEGIN();
    RUN_TEST(test_factory_reset_requires_continuous_five_second_hold);
    RUN_TEST(test_factory_reset_short_press_release_and_bounce_cancel_hold);
    RUN_TEST(test_factory_reset_survives_millis_wrap_and_requires_release_after_reboot);
    RUN_TEST(test_repeated_mode_selection_preserves_previous_strokes);
    RUN_TEST(test_batches_invalidate_renderer_and_reject_invalid_batch_atomically);
    UNITY_END();
}
void loop() {}
