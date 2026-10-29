#include "CarSignalService.h"
#include "pin_definitions.h"
#include "utils/Logger.h"
#include <driver/twai.h>

#define TAG "CAR_SIG"

constexpr bool reservedCanPin(int pin) {
    return pin == PIN_HUB75_R1 || pin == PIN_HUB75_G1 || pin == PIN_HUB75_B1 ||
           pin == PIN_HUB75_R2 || pin == PIN_HUB75_G2 || pin == PIN_HUB75_B2 ||
           pin == PIN_HUB75_A || pin == PIN_HUB75_B || pin == PIN_HUB75_C ||
           pin == PIN_HUB75_D || pin == PIN_HUB75_E || pin == PIN_HUB75_LAT ||
           pin == PIN_HUB75_OE || pin == PIN_HUB75_CLK || pin == PIN_E_STOP ||
           pin == PIN_FACTORY_RESET;
}
static_assert(PIN_CAN_TX != PIN_CAN_RX && !reservedCanPin(PIN_CAN_TX) &&
              !reservedCanPin(PIN_CAN_RX), "CAN pins conflict with LED/buttons");

CarSignalService& CarSignalService::getInstance() {
    static CarSignalService instance;
    return instance;
}

void CarSignalService::init() {
    portENTER_CRITICAL(&stateMux);
    snapshot = VehicleCan::Snapshot{};
    portEXIT_CRITICAL(&stateMux);
    twai_general_config_t general = TWAI_GENERAL_CONFIG_DEFAULT(
        static_cast<gpio_num_t>(PIN_CAN_TX), static_cast<gpio_num_t>(PIN_CAN_RX), TWAI_MODE_NORMAL);
    general.tx_queue_len = 0; // No application TX; NORMAL mode still acknowledges CAN frames.
    general.rx_queue_len = 32;
    general.alerts_enabled = TWAI_ALERT_BUS_OFF | TWAI_ALERT_BUS_RECOVERED | TWAI_ALERT_RX_QUEUE_FULL;
    twai_timing_config_t timing = TWAI_TIMING_CONFIG_500KBITS();
    twai_filter_config_t filter = TWAI_FILTER_CONFIG_ACCEPT_ALL();
    esp_err_t error = twai_driver_install(&general, &timing, &filter);
    if (error == ESP_OK) {
        error = twai_start();
        if (error != ESP_OK) twai_driver_uninstall();
    }
    canReady = error == ESP_OK;
    if (canReady) {
        LOG_I(TAG, "CAN RX ready: TX=%d RX=%d 500k ID=0x321; vehicle UNKNOWN", PIN_CAN_TX, PIN_CAN_RX);
    } else {
        LOG_E(TAG, "CAN init failed: %s; Serial simulator available", esp_err_to_name(error));
    }
    LOG_I(TAG, "Serial L/R/H/S available when CAN is not fresh; ? prints status");
}

VehicleCan::Snapshot CarSignalService::getSnapshot() const {
    portENTER_CRITICAL(&stateMux);
    VehicleCan::Snapshot copy = snapshot;
    portEXIT_CRITICAL(&stateMux);
    // Core 1 renderer must expire state even when Core 0 networking is delayed.
    if (VehicleCan::expired(copy, millis())) return VehicleCan::Snapshot{};
    return copy;
}

void CarSignalService::update() {
    if (canReady) {
        uint32_t alerts = 0;
        twai_read_alerts(&alerts, 0);
        if (alerts & TWAI_ALERT_BUS_OFF) {
            twai_clear_receive_queue();
            portENTER_CRITICAL(&stateMux);
            snapshot = VehicleCan::Snapshot{};
            portEXIT_CRITICAL(&stateMux);
            LOG_W(TAG, "CAN bus-off: vehicle UNKNOWN, recovering");
            twai_initiate_recovery();
        }
        if (alerts & TWAI_ALERT_BUS_RECOVERED) LOG_I(TAG, "CAN restart: %s", esp_err_to_name(twai_start()));
        if (alerts & TWAI_ALERT_RX_QUEUE_FULL) LOG_W(TAG, "CAN RX queue full");
        twai_message_t frame;
        for (int n = 0; n < 32 && twai_receive(&frame, 0) == ESP_OK; ++n) {
            VehicleCan::Snapshot next;
            if (!VehicleCan::decode(frame.identifier, frame.extd, frame.rtr,
                                    frame.data_length_code, frame.data, millis(), next)) continue;
            const auto old = getSnapshot();
            portENTER_CRITICAL(&stateMux);
            snapshot = next;
            portEXIT_CRITICAL(&stateMux);
            staleReported = false;
            if (!old.known || next.turn != old.turn || next.brake != old.brake ||
                next.reverse != old.reverse || millis() - lastLog >= 1000) {
                lastLog = millis();
                LOG_I(TAG, "CAN turn=%u brake=%u reverse=%u speed=%.1f stopped=%u counter=%u",
                      next.turn, next.brake, next.reverse, next.speed / 10.0,
                      next.speed == 0, next.counter);
            }
        }
    }
    portENTER_CRITICAL(&stateMux);
    const auto raw = snapshot;
    portEXIT_CRITICAL(&stateMux);
    if (VehicleCan::expired(raw, millis()) && !staleReported) {
        staleReported = true;
        LOG_W(TAG, "CAN STALE (>500ms): vehicle UNKNOWN, restoring background effect");
    }
    // Bounded processing keeps the CAN receiver and network services responsive.
    for (int n = 0; n < 32 && Serial.available(); ++n) {
        const char ch = static_cast<char>(Serial.read());
        switch (ch) {
            case 'L': case 'l': setState(TurnSignalState::LEFT); break;
            case 'R': case 'r': setState(TurnSignalState::RIGHT); break;
            case 'H': case 'h': setState(TurnSignalState::HAZARD); break;
            case 'S': case 's': case '0': setState(TurnSignalState::IDLE); break;
            case '?': {
                const auto s = getSnapshot();
                LOG_I(TAG, "source=%s signal=%s brake=%u reverse=%u speed=%.1f",
                      !s.known ? "UNKNOWN" : s.fromCan ? "CAN" : "SERIAL",
                      getStateName(), s.brake, s.reverse, s.speed / 10.0);
                break;
            }
            default: break;
        }
    }
}

TurnSignalState CarSignalService::getState() const {
    const auto s = getSnapshot();
    return s.known ? static_cast<TurnSignalState>(s.turn) : TurnSignalState::IDLE;
}
void CarSignalService::setState(TurnSignalState state) {
    const auto current = getSnapshot();
    if (current.known && current.fromCan) return;
    VehicleCan::Snapshot manual;
    manual.known = true;
    manual.turn = static_cast<uint8_t>(state);
    portENTER_CRITICAL(&stateMux);
    snapshot = manual;
    portEXIT_CRITICAL(&stateMux);
    LOG_I(TAG, "Serial signal: %s", getStateName());
}
bool CarSignalService::isActive() const {
    return VehicleCan::display(getSnapshot()) != VehicleCan::Display::Normal;
}
const char* CarSignalService::getStateName() const {
    switch (getState()) {
        case TurnSignalState::LEFT: return "LEFT";
        case TurnSignalState::RIGHT: return "RIGHT";
        case TurnSignalState::HAZARD: return "HAZARD";
        default: return "IDLE";
    }
}
