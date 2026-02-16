#include <gtest/gtest.h>

#include "br_logger/log_level.hpp"

using br_logger::LogLevel;
using br_logger::to_short_char;
using br_logger::to_string;

TEST(LogLevel, ToStringReturnsCorrectValues)
{
  static_assert(to_string(LogLevel::Trace) == "TRACE");
  static_assert(to_string(LogLevel::Debug) == "DEBUG");
  static_assert(to_string(LogLevel::Info) == "INFO");
  static_assert(to_string(LogLevel::Warn) == "WARN");
  static_assert(to_string(LogLevel::Error) == "ERROR");
  static_assert(to_string(LogLevel::Fatal) == "FATAL");
  static_assert(to_string(LogLevel::Off) == "OFF");

  EXPECT_EQ(to_string(LogLevel::Info), "INFO");
}

TEST(LogLevel, ToShortCharReturnsCorrectValues)
{
  static_assert(to_short_char(LogLevel::Trace) == 'T');
  static_assert(to_short_char(LogLevel::Debug) == 'D');
  static_assert(to_short_char(LogLevel::Info) == 'I');
  static_assert(to_short_char(LogLevel::Warn) == 'W');
  static_assert(to_short_char(LogLevel::Error) == 'E');
  static_assert(to_short_char(LogLevel::Fatal) == 'F');

  EXPECT_EQ(to_short_char(LogLevel::Error), 'E');
}

TEST(LogLevel, EnumValuesAreOrdered)
{
  static_assert(static_cast<uint8_t>(LogLevel::Trace) <
                static_cast<uint8_t>(LogLevel::Debug));
  static_assert(static_cast<uint8_t>(LogLevel::Debug) <
                static_cast<uint8_t>(LogLevel::Info));
  static_assert(static_cast<uint8_t>(LogLevel::Info) <
                static_cast<uint8_t>(LogLevel::Warn));
  static_assert(static_cast<uint8_t>(LogLevel::Warn) <
                static_cast<uint8_t>(LogLevel::Error));
  static_assert(static_cast<uint8_t>(LogLevel::Error) <
                static_cast<uint8_t>(LogLevel::Fatal));
  static_assert(static_cast<uint8_t>(LogLevel::Fatal) <
                static_cast<uint8_t>(LogLevel::Off));

  EXPECT_TRUE(true);
}

TEST(LogLevel, ActiveLevelMacroDefined)
{
  EXPECT_GE(BR_LOG_ACTIVE_LEVEL, 0);
  EXPECT_LE(BR_LOG_ACTIVE_LEVEL, 6);
}
