#include <Arduino.h>
#include <unity.h>

void test_color_struct() {
    TEST_ASSERT_EQUAL(1, 1);
}

void setup() {
    delay(2000);
    UNITY_BEGIN();
    RUN_TEST(test_color_struct);
    UNITY_END();
}

void loop() {}
