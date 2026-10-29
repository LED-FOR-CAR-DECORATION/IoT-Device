#pragma once
#include <stdint.h>

// Kan_sim bench protocol v1, NOT Hyundai CAN identifiers.
namespace VehicleCan {
constexpr uint32_t ID = 0x321;
constexpr uint32_t TIMEOUT_MS = 500;
struct Snapshot {
    bool known = false;
    bool fromCan = false;
    uint8_t turn = 0;
    bool reverse = false;
    bool brake = false;
    uint16_t speed = 0; // 0.1 km/h
    uint8_t counter = 0;
    uint32_t receivedAt = 0;
};
inline bool decode(uint32_t id, bool extended, bool remote, uint8_t length,
                   const uint8_t* data, uint32_t now, Snapshot& result) {
    if (id != ID || extended || remote || length != 8 || !data) return false;
    if (data[0] != 1 || (data[1] & 0xf0) || data[5] || data[6] || data[7]) return false;
    const uint16_t speed = data[2] | (uint16_t(data[3]) << 8);
    if (speed > 2000) return false;
    Snapshot next;
    next.known = next.fromCan = true;
    next.turn = data[1] & 3;
    next.reverse = (data[1] & 4) != 0;
    next.brake = (data[1] & 8) != 0;
    next.speed = speed;
    next.counter = data[4];
    next.receivedAt = now;
    result = next;
    return true;
}
inline bool expired(const Snapshot& state, uint32_t now) {
    return state.known && state.fromCan && uint32_t(now - state.receivedAt) > TIMEOUT_MS;
}
enum class Display : uint8_t { Normal, Left, Right, Hazard, Brake, Reverse, Stopped };
inline Display display(const Snapshot& s) {
    if (!s.known) return Display::Normal;
    if (s.turn == 3) return Display::Hazard;
    if (s.turn == 1) return Display::Left;
    if (s.turn == 2) return Display::Right;
    if (!s.fromCan) return Display::Normal;
    if (s.brake) return Display::Brake;
    if (s.reverse) return Display::Reverse;
    if (s.speed == 0) return Display::Stopped;
    return Display::Normal;
}
}
