#include "Logger.h"
#include <stdarg.h>

void Logger::init(unsigned long baudRate) {
    Serial.begin(baudRate);
    delay(50);
}

const char* Logger::levelToString(LogLevel level) {
    switch (level) {
        case LogLevel::DEBUG_LVL: return "DEBUG";
        case LogLevel::INFO_LVL:  return "INFO ";
        case LogLevel::WARN_LVL:  return "WARN ";
        case LogLevel::ERROR_LVL: return "ERROR";
        default:                  return "LOG  ";
    }
}

void Logger::log(LogLevel level, const char* tag, const char* format, ...) {
    char buffer[256];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    unsigned long ms = millis();
    Serial.printf("[%6lu.%03lu][%s][%s] %s\n", 
                  ms / 1000, 
                  ms % 1000, 
                  levelToString(level), 
                  tag, 
                  buffer);
}
