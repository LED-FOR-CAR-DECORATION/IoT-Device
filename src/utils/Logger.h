#pragma once

#include <Arduino.h>

enum class LogLevel {
    DEBUG_LVL,
    INFO_LVL,
    WARN_LVL,
    ERROR_LVL
};

class Logger {
public:
    static void init(unsigned long baudRate = 115200);
    static void log(LogLevel level, const char* tag, const char* format, ...);

private:
    static const char* levelToString(LogLevel level);
};

#define LOG_D(tag, format, ...) Logger::log(LogLevel::DEBUG_LVL, tag, format, ##__VA_ARGS__)
#define LOG_I(tag, format, ...) Logger::log(LogLevel::INFO_LVL,  tag, format, ##__VA_ARGS__)
#define LOG_W(tag, format, ...) Logger::log(LogLevel::WARN_LVL,  tag, format, ##__VA_ARGS__)
#define LOG_E(tag, format, ...) Logger::log(LogLevel::ERROR_LVL, tag, format, ##__VA_ARGS__)
