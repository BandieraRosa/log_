#include <gtest/gtest.h>

#include "br_logger/log_level.hpp"

using br_logger::LogLevel;
using br_logger::to_short_char;
using br_logger::to_string;

TEST(LogLevel, ToStringReturnsCorrectValues)
{
  static_assert(to_string(LogLevel::TRACE) == "TRACE");
  static_assert(to_string(LogLevel::DEBUG) == "DEBUG");
  static_assert(to_string(LogLevel::INFO) == "INFO");
  static_assert(to_string(LogLevel::WARN) == "WARN");
  static_assert(to_string(LogLevel::ERROR) == "ERROR");
  static_assert(to_string(LogLevel::FATAL) == "FATAL");
  static_assert(to_string(LogLevel::OFF) == "OFF");

  EXPECT_EQ(to_string(LogLevel::INFO), "INFO");
}

TEST(LogLevel, ToShortCharReturnsCorrectValues)
{
  static_assert(to_short_char(LogLevel::TRACE) == 'T');
  static_assert(to_short_char(LogLevel::DEBUG) == 'D');
  static_assert(to_short_char(LogLevel::INFO) == 'I');
  static_assert(to_short_char(LogLevel::WARN) == 'W');
  static_assert(to_short_char(LogLevel::ERROR) == 'E');
  static_assert(to_short_char(LogLevel::FATAL) == 'F');

  EXPECT_EQ(to_short_char(LogLevel::ERROR), 'E');
}

TEST(LogLevel, EnumValuesAreOrdered)
{
  static_assert(static_cast<uint8_t>(LogLevel::TRACE) <
                static_cast<uint8_t>(LogLevel::DEBUG));
  static_assert(static_cast<uint8_t>(LogLevel::DEBUG) <
                static_cast<uint8_t>(LogLevel::INFO));
  static_assert(static_cast<uint8_t>(LogLevel::INFO) <
                static_cast<uint8_t>(LogLevel::WARN));
  static_assert(static_cast<uint8_t>(LogLevel::WARN) <
                static_cast<uint8_t>(LogLevel::ERROR));
  static_assert(static_cast<uint8_t>(LogLevel::ERROR) <
                static_cast<uint8_t>(LogLevel::FATAL));
  static_assert(static_cast<uint8_t>(LogLevel::FATAL) <
                static_cast<uint8_t>(LogLevel::OFF));

  EXPECT_TRUE(true);
}

TEST(LogLevel, ActiveLevelMacroDefined)
{
  EXPECT_GE(BR_LOG_ACTIVE_LEVEL, 0);
  EXPECT_LE(BR_LOG_ACTIVE_LEVEL, 6);
}
