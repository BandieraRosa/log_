#include <gtest/gtest.h>

#include <cstring>
#include <string>
#include <vector>

#include "../include/br_logger/log_entry.hpp"
#include "../include/br_logger/log_level.hpp"
#include "../include/br_logger/sinks/callback_sink.hpp"

static br_logger::LogEntry make_test_entry(
    br_logger::LogLevel level = br_logger::LogLevel::Info,
    const char* msg = "test message")
{
  br_logger::LogEntry entry{};
  entry.wall_clock_ns = 1739692200123456000ULL;
  entry.timestamp_ns = 123456789ULL;
  entry.level = level;
  entry.file_path = "/src/main.cpp";
  entry.file_name = "main.cpp";
  entry.function_name = "process";
  entry.pretty_function = "void process(int)";
  entry.line = 42;
  entry.column = 0;
  entry.thread_id = 1234;
  entry.process_id = 5678;
  std::strncpy(entry.thread_name, "worker", sizeof(entry.thread_name));
  entry.tag_count = 0;
  entry.sequence_id = 1001;
  entry.msg_len = static_cast<uint16_t>(std::strlen(msg));
  std::strncpy(entry.msg, msg, BR_LOG_MAX_MSG_LEN);
  return entry;
}

TEST(CallbackSink, CallbackInvokedOnWrite)
{
  bool called = false;
  br_logger::CallbackSink sink([&](const br_logger::LogEntry&) { called = true; });

  auto entry = make_test_entry();
  sink.Write(entry);
  EXPECT_TRUE(called);
}

TEST(CallbackSink, LogEntryFieldsPassedCorrectly)
{
  br_logger::LogEntry captured{};
  br_logger::CallbackSink sink([&](const br_logger::LogEntry& e) { captured = e; });

  auto entry = make_test_entry(br_logger::LogLevel::Warn, "hello world");
  sink.Write(entry);

  EXPECT_EQ(captured.level, br_logger::LogLevel::Warn);
  EXPECT_EQ(std::string(captured.msg, captured.msg_len), "hello world");
  EXPECT_EQ(captured.thread_id, 1234u);
  EXPECT_EQ(captured.line, 42u);
  EXPECT_STREQ(captured.file_name, "main.cpp");
  EXPECT_STREQ(captured.function_name, "process");
  EXPECT_EQ(captured.sequence_id, 1001u);
}

TEST(CallbackSink, MultipleWritesInvokeCallbackMultipleTimes)
{
  int count = 0;
  br_logger::CallbackSink sink([&](const br_logger::LogEntry&) { ++count; });

  for (int i = 0; i < 5; ++i)
  {
    auto entry = make_test_entry();
    sink.Write(entry);
  }
  EXPECT_EQ(count, 5);
}

TEST(CallbackSink, LevelFilteringBlocksLowLevel)
{
  bool called = false;
  br_logger::CallbackSink sink([&](const br_logger::LogEntry&) { called = true; });
  sink.SetLevel(br_logger::LogLevel::Warn);

  auto entry = make_test_entry(br_logger::LogLevel::Info);
  sink.Write(entry);
  EXPECT_FALSE(called);
}

TEST(CallbackSink, LevelFilteringAllowsHighLevel)
{
  bool called = false;
  br_logger::CallbackSink sink([&](const br_logger::LogEntry&) { called = true; });
  sink.SetLevel(br_logger::LogLevel::Warn);

  auto entry = make_test_entry(br_logger::LogLevel::Error);
  sink.Write(entry);
  EXPECT_TRUE(called);
}

TEST(CallbackSink, LevelFilteringAllowsExactLevel)
{
  bool called = false;
  br_logger::CallbackSink sink([&](const br_logger::LogEntry&) { called = true; });
  sink.SetLevel(br_logger::LogLevel::Warn);

  auto entry = make_test_entry(br_logger::LogLevel::Warn);
  sink.Write(entry);
  EXPECT_TRUE(called);
}

TEST(CallbackSink, AccumulatorCallback)
{
  std::vector<std::string> messages;
  br_logger::CallbackSink sink([&](const br_logger::LogEntry& e)
                               { messages.emplace_back(e.msg, e.msg_len); });

  sink.Write(make_test_entry(br_logger::LogLevel::Info, "msg1"));
  sink.Write(make_test_entry(br_logger::LogLevel::Info, "msg2"));
  sink.Write(make_test_entry(br_logger::LogLevel::Info, "msg3"));

  ASSERT_EQ(messages.size(), 3u);
  EXPECT_EQ(messages[0], "msg1");
  EXPECT_EQ(messages[1], "msg2");
  EXPECT_EQ(messages[2], "msg3");
}

TEST(CallbackSink, NullCallbackDoesNotCrash)
{
  br_logger::CallbackSink sink(nullptr);
  auto entry = make_test_entry();
  EXPECT_NO_THROW(sink.Write(entry));
}

TEST(CallbackSink, EmptyStdFunctionDoesNotCrash)
{
  br_logger::CallbackSink::Callback empty_cb;
  br_logger::CallbackSink sink(empty_cb);
  auto entry = make_test_entry();
  EXPECT_NO_THROW(sink.Write(entry));
}

TEST(CallbackSink, FlushDoesNotCrash)
{
  br_logger::CallbackSink sink([](const br_logger::LogEntry&) {});
  EXPECT_NO_THROW(sink.Flush());
}

TEST(CallbackSink, DefaultLevelIsTrace)
{
  br_logger::CallbackSink sink([](const br_logger::LogEntry&) {});
  EXPECT_EQ(sink.Level(), br_logger::LogLevel::Trace);
}
