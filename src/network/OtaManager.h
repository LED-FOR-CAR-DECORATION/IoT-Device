#pragma once

#include <Arduino.h>
#include <atomic>

class OtaManager {
public:
    static OtaManager& getInstance();
    void earlyBoot();
    void sendInfo(const String& requestId);
    void start(const String& token, const String& requestId);
    bool isBusy() const { return busy.load(); }

    void init();
    void update();

private:
    OtaManager() = default;
    static void worker(void* argument);
    void run();
    void fail(const char* code);
    void rotateNonce();
    std::atomic<bool> busy{false};
    std::atomic<int> progress{0}, result{0};
    String token, nonce, sessionNonce, requestId;
    char failure[48] = {};
    int lastProgress = -1;
    unsigned long healthySince = 0;
};
