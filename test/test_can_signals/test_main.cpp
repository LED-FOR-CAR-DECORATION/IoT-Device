#include <Arduino.h>
#include <unity.h>
#include "VehicleCanProtocol.h"

void test_combined_frame_and_counter() {
    uint8_t data[] = {1, 15, 55, 0, 255, 0, 0, 0};
    VehicleCan::Snapshot s;
    TEST_ASSERT_TRUE(VehicleCan::decode(0x321, false, false, 8, data, 100, s));
    TEST_ASSERT_TRUE(s.known && s.fromCan && s.reverse && s.brake);
    TEST_ASSERT_EQUAL(3, s.turn);
    TEST_ASSERT_EQUAL(55, s.speed);
    TEST_ASSERT_EQUAL(255, s.counter);
    data[4] = 0;
    TEST_ASSERT_TRUE(VehicleCan::decode(0x321, false, false, 8, data, 200, s));
    TEST_ASSERT_EQUAL(0, s.counter);
}
void test_bad_frames_preserve_state() {
    uint8_t data[] = {1, 1, 0xd0, 7, 2, 0, 0, 0};
    VehicleCan::Snapshot s;
    TEST_ASSERT_TRUE(VehicleCan::decode(0x321, false, false, 8, data, 100, s));
    TEST_ASSERT_FALSE(VehicleCan::decode(0x322, false, false, 8, data, 300, s));
    TEST_ASSERT_FALSE(VehicleCan::decode(0x321, true, false, 8, data, 300, s));
    TEST_ASSERT_FALSE(VehicleCan::decode(0x321, false, true, 8, data, 300, s));
    TEST_ASSERT_FALSE(VehicleCan::decode(0x321, false, false, 7, data, 300, s));
    for (int index : {0, 1, 5, 6, 7}) {
        const uint8_t saved = data[index];
        data[index] = index == 1 ? 0x80 : 2;
        TEST_ASSERT_FALSE(VehicleCan::decode(0x321, false, false, 8, data, 300, s));
        data[index] = saved;
    }
    data[2] = 0xd1; // 200.1 km/h is outside v1 range.
    TEST_ASSERT_FALSE(VehicleCan::decode(0x321, false, false, 8, data, 300, s));
    TEST_ASSERT_EQUAL(100, s.receivedAt);
    TEST_ASSERT_EQUAL(2000, s.speed);
}
void test_timeout_and_millis_wrap() {
    VehicleCan::Snapshot s;
    TEST_ASSERT_FALSE(VehicleCan::expired(s, 1000));
    s.known = s.fromCan = true;
    s.receivedAt = UINT32_MAX - 100;
    TEST_ASSERT_FALSE(VehicleCan::expired(s, 399)); // exactly 500 ms
    TEST_ASSERT_TRUE(VehicleCan::expired(s, 400));
    s.fromCan = false;
    TEST_ASSERT_FALSE(VehicleCan::expired(s, 10000));
}
void test_display_priorities_and_unknown() {
    using namespace VehicleCan;
    Snapshot s;
    TEST_ASSERT_EQUAL(int(Display::Normal), int(display(s)));
    s.known = s.fromCan = true;
    TEST_ASSERT_EQUAL(int(Display::Stopped), int(display(s)));
    s.reverse = true;
    TEST_ASSERT_EQUAL(int(Display::Reverse), int(display(s)));
    s.brake = true;
    TEST_ASSERT_EQUAL(int(Display::Brake), int(display(s)));
    s.turn = 3;
    TEST_ASSERT_EQUAL(int(Display::Hazard), int(display(s)));
    s.turn = 1;
    TEST_ASSERT_EQUAL(int(Display::Left), int(display(s)));
    s.turn = 2;
    TEST_ASSERT_EQUAL(int(Display::Right), int(display(s)));
    s = Snapshot{}; s.known = true; // Serial idle must never become STOP.
    TEST_ASSERT_EQUAL(int(Display::Normal), int(display(s)));
    s.fromCan = true; s.speed = 100;
    TEST_ASSERT_EQUAL(int(Display::Normal), int(display(s)));
}
void setup() {
    delay(2000);
    UNITY_BEGIN();
    RUN_TEST(test_combined_frame_and_counter);
    RUN_TEST(test_bad_frames_preserve_state);
    RUN_TEST(test_timeout_and_millis_wrap);
    RUN_TEST(test_display_priorities_and_unknown);
    UNITY_END();
}
void loop() {}
