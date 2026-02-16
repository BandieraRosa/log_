#pragma once
#include <cstdint>
#include <string_view>

namespace br_logger {

enum class LogLevel : uint8_t {
    Trace = 0,
    Debug = 1,
    Info  = 2,
    Warn  = 3,
    Error = 4,
    Fatal = 5,
    Off   = 6
};

constexpr std::string_view to_string(LogLevel level) {
    switch (level) {
        case LogLevel::Trace: return "TRACE";
        case LogLevel::Debug: return "DEBUG";
        case LogLevel::Info:  return "INFO";
        case LogLevel::Warn:  return "WARN";
        case LogLevel::Error: return "ERROR";
        case LogLevel::Fatal: return "FATAL";
        case LogLevel::Off:   return "OFF";
    }
    return "UNKNOWN";
}

constexpr char to_short_char(LogLevel level) {
    switch (level) {
        case LogLevel::Trace: return 'T';
        case LogLevel::Debug: return 'D';
        case LogLevel::Info:  return 'I';
        case LogLevel::Warn:  return 'W';
        case LogLevel::Error: return 'E';
        case LogLevel::Fatal: return 'F';
        case LogLevel::Off:   return 'O';
    }
    return '?';
}

// 编译期最低活跃级别（通过 CMake -DBR_LOG_ACTIVE_LEVEL=2 注入）
#ifndef BR_LOG_ACTIVE_LEVEL
    #ifdef NDEBUG
        #define BR_LOG_ACTIVE_LEVEL 2  // Info
    #else
        #define BR_LOG_ACTIVE_LEVEL 0  // Trace
    #endif
#endif

} // namespace br_logger
