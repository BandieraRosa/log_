#include <gtest/gtest.h>

#include <cstring>
#include <regex>

#include "br_logger/timestamp.hpp"

using namespace br_logger;

TEST(Timestamp, MonotonicIsIncreasing)
{
  uint64_t t1 = monotonic_now_ns();
  uint64_t t2 = monotonic_now_ns();
  EXPECT_GE(t2, t1);
}

TEST(Timestamp, WallClockReasonableRange)
{
  uint64_t now = wall_clock_now_ns();
  uint64_t year_2020_ns = 1577836800ULL * 1'000'000'000ULL;
  uint64_t year_2100_ns = 4102444800ULL * 1'000'000'000ULL;
  EXPECT_GT(now, year_2020_ns);
  EXPECT_LT(now, year_2100_ns);
}

TEST(Timestamp, FormatTimestampMatchesPattern)
{
  uint64_t now = wall_clock_now_ns();
  char buf[64]{};
  size_t len = format_timestamp(now, buf, sizeof(buf));
  EXPECT_GT(len, 0u);

  std::regex pattern(R"(\d{4}-\d{2}-\d{2} \d{2}:\d{2}:\d{2}\.\d{6})");
  EXPECT_TRUE(std::regex_match(buf, pattern)) << "Actual: " << buf;
}

TEST(Timestamp, FormatDateMatchesPattern)
{
  uint64_t now = wall_clock_now_ns();
  char buf[32]{};
  size_t len = format_date(now, buf, sizeof(buf));
  EXPECT_GT(len, 0u);
  EXPECT_EQ(len, 10u);

  std::regex pattern(R"(\d{4}-\d{2}-\d{2})");
  EXPECT_TRUE(std::regex_match(buf, pattern)) << "Actual: " << buf;
}

TEST(Timestamp, FormatTimeMatchesPattern)
{
  uint64_t now = wall_clock_now_ns();
  char buf[32]{};
  size_t len = format_time(now, buf, sizeof(buf));
  EXPECT_GT(len, 0u);

  std::regex pattern(R"(\d{2}:\d{2}:\d{2}\.\d{6})");
  EXPECT_TRUE(std::regex_match(buf, pattern)) << "Actual: " << buf;
}

TEST(Timestamp, FormatTimestampSmallBuffer)
{
  uint64_t now = wall_clock_now_ns();
  char buf[8]{};
  size_t len = format_timestamp(now, buf, sizeof(buf));
  EXPECT_LE(len, 7u);
  EXPECT_EQ(buf[7], '\0');
}

TEST(Timestamp, FormatTimestampZeroBuffer)
{
  uint64_t now = wall_clock_now_ns();
  size_t len = format_timestamp(now, nullptr, 0);
  EXPECT_EQ(len, 0u);
}

TEST(Timestamp, FormatKnownTimestamp)
{
  uint64_t ts = 1708099200ULL * 1'000'000'000ULL + 123456000ULL;
  char buf[64]{};
  format_timestamp(ts, buf, sizeof(buf));
  EXPECT_NE(std::strstr(buf, ".123456"), nullptr)
      << "Expected microseconds .123456, got: " << buf;
}
