#pragma once

#include <Arduino.h>
#include <functional>

class ButtonDriver {
public:
    using ClickCallback = std::function<void()>;
    using LongPressCallback = std::function<void()>;

    static ButtonDriver& getInstance();

    void init();
    void update();
    void onClick(ClickCallback cb);
    void onLongPress(LongPressCallback cb);

private:
    ButtonDriver();
    ClickCallback clickCb;
    LongPressCallback longPressCb;

    bool lastState;
    unsigned long pressStartTime;
    bool isLongPressed;
};
