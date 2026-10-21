#pragma once

namespace FactoryReset {
// Called before state, storage, BLE, Wi-Fi or render tasks are initialized.
bool completePending();
// Only records intent and restarts. Destructive work runs on the next boot,
// with no concurrent BLE uploads, renderer or Wi-Fi writes.
void request();
}
