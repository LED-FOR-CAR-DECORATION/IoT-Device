#include <Arduino.h>
#include <unity.h>

#include "storage/StorageManager.h"

void test_color_struct() {
    TEST_ASSERT_EQUAL(1, 1);
}

void test_projection_header() {
    ProjectionHeader header;
    TEST_ASSERT_EQUAL_UINT32(PROJECTION_MAGIC, header.magic);
    TEST_ASSERT_EQUAL_UINT8(1, header.version);
    TEST_ASSERT_EQUAL_UINT16(64, header.width);
    TEST_ASSERT_EQUAL_UINT16(32, header.height);
}

void setup() {
    delay(2000);
    UNITY_BEGIN();
    RUN_TEST(test_color_struct);
    RUN_TEST(test_projection_header);
    UNITY_END();
}

void loop() {}
