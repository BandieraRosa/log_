#include <gtest/gtest.h>

#include <cstring>
#include <string>

#include "../include/br_logger/formatters/pattern_formatter.hpp"
#include "../include/br_logger/log_entry.hpp"
#include "../include/br_logger/log_level.hpp"
#include "../include/br_logger/platform.hpp"

static br_logger::LogEntry make_test_entry()
{
  br_logger::LogEntry entry{};
  entry.wall_clock_ns = 1739692200123456000ULL;
  entry.timestamp_ns = 123456789ULL;
  entry.level = br_logger::LogLevel::Info;
  entry.file_path = "/src/main.cpp";
  entry.file_name = "main.cpp";
  entry.function_name = "process";
  entry.pretty_function = "void process(int)";
  entry.line = 42;
  entry.column = 0;
  entry.thread_id = 1234;
  entry.process_id = 5678;
  std::strncpy(entry.thread_name, "worker", sizeof(entry.thread_name));
  entry.tag_count = 2;
  std::strncpy(entry.tags[0].key, "env", BR_LOG_MAX_TAG_KEY_LEN);
  std::strncpy(entry.tags[0].value, "prod", BR_LOG_MAX_TAG_VAL_LEN);
  std::strncpy(entry.tags[1].key, "req", BR_LOG_MAX_TAG_KEY_LEN);
  std::strncpy(entry.tags[1].value, "abc123", BR_LOG_MAX_TAG_VAL_LEN);
  entry.sequence_id = 1001;
  const char* msg = "hello world";
  entry.msg_len = static_cast<uint16_t>(std::strlen(msg));
  std::strncpy(entry.msg, msg, BR_LOG_MAX_MSG_LEN);
  return entry;
}

static std::string format_with(const br_logger::LogEntry& entry, const char* pattern,
                               bool enable_color = true)
{
  br_logger::PatternFormatter formatter(pattern, enable_color);
  char buf[512];
  formatter.Format(entry, buf, sizeof(buf));
  return std::string(buf);
}

TEST(PatternFormatter, LevelFull)
{
  auto entry = make_test_entry();
  auto out = format_with(entry, "%L");
  EXPECT_NE(out.find("INFO"), std::string::npos);
}

TEST(PatternFormatter, LevelShort)
{
  auto entry = make_test_entry();
  auto out = format_with(entry, "%l");
  EXPECT_NE(out.find('I'), std::string::npos);
}

TEST(PatternFormatter, FileName)
{
  auto entry = make_test_entry();
  auto out = format_with(entry, "%f");
  EXPECT_NE(out.find("main.cpp"), std::string::npos);
}

TEST(PatternFormatter, FilePath)
{
  auto entry = make_test_entry();
  auto out = format_with(entry, "%F");
  EXPECT_NE(out.find("/src/main.cpp"), std::string::npos);
}

TEST(PatternFormatter, FuncName)
{
  auto entry = make_test_entry();
  auto out = format_with(entry, "%n");
  EXPECT_NE(out.find("process"), std::string::npos);
}

TEST(PatternFormatter, PrettyFunc)
{
  auto entry = make_test_entry();
  auto out = format_with(entry, "%N");
  EXPECT_NE(out.find("void process(int)"), std::string::npos);
}

TEST(PatternFormatter, Line)
{
  auto entry = make_test_entry();
  auto out = format_with(entry, "%#");
  EXPECT_NE(out.find("42"), std::string::npos);
}

TEST(PatternFormatter, ThreadId)
{
  auto entry = make_test_entry();
  auto out = format_with(entry, "%t");
  EXPECT_NE(out.find("1234"), std::string::npos);
}

TEST(PatternFormatter, ProcessId)
{
  auto entry = make_test_entry();
  auto out = format_with(entry, "%P");
  EXPECT_NE(out.find("5678"), std::string::npos);
}

TEST(PatternFormatter, ThreadName)
{
  auto entry = make_test_entry();
  auto out = format_with(entry, "%k");
  EXPECT_NE(out.find("worker"), std::string::npos);
}

TEST(PatternFormatter, SequenceId)
{
  auto entry = make_test_entry();
  auto out = format_with(entry, "%q");
  EXPECT_NE(out.find("1001"), std::string::npos);
}

TEST(PatternFormatter, Tags)
{
  auto entry = make_test_entry();
  auto out = format_with(entry, "%g");
  EXPECT_NE(out.find("[env=prod|req=abc123]"), std::string::npos);
}

TEST(PatternFormatter, Message)
{
  auto entry = make_test_entry();
  auto out = format_with(entry, "%m");
  EXPECT_NE(out.find("hello world"), std::string::npos);
}

TEST(PatternFormatter, EscapedPercent)
{
  auto entry = make_test_entry();
  auto out = format_with(entry, "100%%");
  EXPECT_EQ(out, "100%");
}

TEST(PatternFormatter, Literal)
{
  auto entry = make_test_entry();
  auto out = format_with(entry, "hello");
  EXPECT_EQ(out, "hello");
}

TEST(PatternFormatter, Date)
{
  auto entry = make_test_entry();
  auto out = format_with(entry, "%D");
  EXPECT_EQ(out.size(), 10u);
  EXPECT_EQ(out[4], '-');
  EXPECT_EQ(out[7], '-');
}

TEST(PatternFormatter, Time)
{
  auto entry = make_test_entry();
  auto out = format_with(entry, "%T");
  EXPECT_EQ(out.size(), 8u);
  EXPECT_EQ(out[2], ':');
  EXPECT_EQ(out[5], ':');
}

TEST(PatternFormatter, Microseconds)
{
  auto entry = make_test_entry();
  auto out = format_with(entry, "%e");
  EXPECT_EQ(out.size(), 7u);
  EXPECT_EQ(out[0], '.');
}

TEST(PatternFormatter, ColorInfo)
{
  auto entry = make_test_entry();
  entry.level = br_logger::LogLevel::Info;
  auto out = format_with(entry, "%C%L%R", true);
  EXPECT_NE(out.find("\033[32m"), std::string::npos);
  EXPECT_NE(out.find("\033[0m"), std::string::npos);
}

TEST(PatternFormatter, ColorWarn)
{
  auto entry = make_test_entry();
  entry.level = br_logger::LogLevel::Warn;
  auto out = format_with(entry, "%C%L%R", true);
  EXPECT_NE(out.find("\033[33m"), std::string::npos);
}

TEST(PatternFormatter, ColorError)
{
  auto entry = make_test_entry();
  entry.level = br_logger::LogLevel::Error;
  auto out = format_with(entry, "%C%L%R", true);
  EXPECT_NE(out.find("\033[31m"), std::string::npos);
}

TEST(PatternFormatter, ColorFatal)
{
  auto entry = make_test_entry();
  entry.level = br_logger::LogLevel::Fatal;
  auto out = format_with(entry, "%C%L%R", true);
  EXPECT_NE(out.find("\033[1;31m"), std::string::npos);
}

TEST(PatternFormatter, ColorDisabled)
{
  auto entry = make_test_entry();
  entry.level = br_logger::LogLevel::Info;
  auto out = format_with(entry, "%C%L%R", false);
  EXPECT_EQ(out.find("\033["), std::string::npos);
}

TEST(PatternFormatter, DefaultPattern)
{
  auto entry = make_test_entry();
  br_logger::PatternFormatter formatter;
  char buf[512];
  formatter.Format(entry, buf, sizeof(buf));
  std::string out(buf);
  EXPECT_FALSE(out.empty());
  EXPECT_NE(out.find("main.cpp"), std::string::npos);
  EXPECT_NE(out.find("hello world"), std::string::npos);
}

TEST(PatternFormatter, EmptyTags)
{
  auto entry = make_test_entry();
  entry.tag_count = 0;
  auto out = format_with(entry, "%g");
  EXPECT_TRUE(out.empty());
}

TEST(PatternFormatter, BufferOverflow)
{
  auto entry = make_test_entry();
  br_logger::PatternFormatter formatter("%L %m");
  char buf[5];
  formatter.Format(entry, buf, sizeof(buf));
  EXPECT_EQ(buf[sizeof(buf) - 1], '\0');
}
