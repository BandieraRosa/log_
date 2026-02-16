#pragma once
#include <cstdint>
#include <string_view>

namespace br_logger
{

enum class LogLevel : uint8_t
{
  TRACE = 0,
  DEBUG = 1,
  INFO = 2,
  WARN = 3,
  ERROR = 4,
  FATAL = 5,
  OFF = 6
};

constexpr std::string_view to_string(LogLevel level)
{
  switch (level)
  {
    case LogLevel::TRACE:
      return "TRACE";
    case LogLevel::DEBUG:
      return "DEBUG";
    case LogLevel::INFO:
      return "INFO";
    case LogLevel::WARN:
      return "WARN";
    case LogLevel::ERROR:
      return "ERROR";
    case LogLevel::FATAL:
      return "FATAL";
    case LogLevel::OFF:
      return "OFF";
  }
  return "UNKNOWN";
}

constexpr char to_short_char(LogLevel level)
{
  switch (level)
  {
    case LogLevel::TRACE:
      return 'T';
    case LogLevel::DEBUG:
      return 'D';
    case LogLevel::INFO:
      return 'I';
    case LogLevel::WARN:
      return 'W';
    case LogLevel::ERROR:
      return 'E';
    case LogLevel::FATAL:
      return 'F';
    case LogLevel::OFF:
      return 'O';
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

}  // namespace br_logger
