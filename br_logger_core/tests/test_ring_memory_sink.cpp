#include <gtest/gtest.h>
#include <sys/stat.h>
#include <unistd.h>

#include <cstdio>
#include <cstring>
#include <fstream>
#include <sstream>
#include <string>

#include "../include/br_logger/log_entry.hpp"
#include "../include/br_logger/log_level.hpp"
#include "../include/br_logger/sinks/ring_memory_sink.hpp"

static br_logger::LogEntry make_test_entry(
    br_logger::LogLevel level = br_logger::LogLevel::INFO,
    const char* msg = "test message", uint64_t seq = 1001)
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
  entry.sequence_id = seq;
  entry.msg_len = static_cast<uint16_t>(std::strlen(msg));
  std::strncpy(entry.msg, msg, BR_LOG_MAX_MSG_LEN);
  return entry;
}

class RingMemorySinkTest : public ::testing::Test
{
 protected:
  std::string tmp_dir_;
  std::string tmp_file_;

  void SetUp() override
  {
    char tmpl[] = "/tmp/ring_sink_test_XXXXXX";
    char* dir = mkdtemp(tmpl);
    ASSERT_NE(dir, nullptr);
    tmp_dir_ = dir;
    tmp_file_ = tmp_dir_ + "/dump.log";
  }

  void TearDown() override
  {
    std::remove(tmp_file_.c_str());
    rmdir(tmp_dir_.c_str());
  }

  std::string ReadFile(const std::string& path)
  {
    std::ifstream ifs(path);
    std::ostringstream oss;
    oss << ifs.rdbuf();
    return oss.str();
  }
};

TEST_F(RingMemorySinkTest, WriteAndSize)
{
  br_logger::RingMemorySink sink(10);
  EXPECT_EQ(sink.Size(), 0u);

  sink.Write(make_test_entry());
  EXPECT_EQ(sink.Size(), 1u);

  sink.Write(make_test_entry());
  EXPECT_EQ(sink.Size(), 2u);
}

TEST_F(RingMemorySinkTest, RingWrapsAround)
{
  br_logger::RingMemorySink sink(3);

  for (uint64_t i = 0; i < 5; ++i)
  {
    auto msg = "msg" + std::to_string(i);
    sink.Write(make_test_entry(br_logger::LogLevel::INFO, msg.c_str(), i));
  }

  EXPECT_EQ(sink.Size(), 3u);

  EXPECT_EQ(std::string(sink.At(0).msg, sink.At(0).msg_len), "msg2");
  EXPECT_EQ(std::string(sink.At(1).msg, sink.At(1).msg_len), "msg3");
  EXPECT_EQ(std::string(sink.At(2).msg, sink.At(2).msg_len), "msg4");
}

TEST_F(RingMemorySinkTest, AtReturnsOldestToNewest)
{
  br_logger::RingMemorySink sink(5);

  for (uint64_t i = 0; i < 3; ++i)
  {
    auto msg = "entry" + std::to_string(i);
    sink.Write(make_test_entry(br_logger::LogLevel::INFO, msg.c_str(), i));
  }

  EXPECT_EQ(std::string(sink.At(0).msg, sink.At(0).msg_len), "entry0");
  EXPECT_EQ(std::string(sink.At(1).msg, sink.At(1).msg_len), "entry1");
  EXPECT_EQ(std::string(sink.At(2).msg, sink.At(2).msg_len), "entry2");
}

TEST_F(RingMemorySinkTest, DumpToFileCreatesFormattedOutput)
{
  br_logger::RingMemorySink sink(10);
  sink.Write(make_test_entry(br_logger::LogLevel::INFO, "hello dump"));
  sink.Write(make_test_entry(br_logger::LogLevel::WARN, "warning dump"));

  EXPECT_TRUE(sink.DumpToFile(tmp_file_.c_str()));

  std::string content = ReadFile(tmp_file_);
  EXPECT_NE(content.find("hello dump"), std::string::npos);
  EXPECT_NE(content.find("warning dump"), std::string::npos);
}

TEST_F(RingMemorySinkTest, DumpToFileEmptyRing)
{
  br_logger::RingMemorySink sink(10);
  EXPECT_TRUE(sink.DumpToFile(tmp_file_.c_str()));

  std::string content = ReadFile(tmp_file_);
  EXPECT_TRUE(content.empty());
}

TEST_F(RingMemorySinkTest, LevelFilteringWorks)
{
  br_logger::RingMemorySink sink(10);
  sink.SetLevel(br_logger::LogLevel::WARN);

  sink.Write(make_test_entry(br_logger::LogLevel::INFO, "filtered"));
  EXPECT_EQ(sink.Size(), 0u);

  sink.Write(make_test_entry(br_logger::LogLevel::WARN, "allowed"));
  EXPECT_EQ(sink.Size(), 1u);
  EXPECT_EQ(std::string(sink.At(0).msg, sink.At(0).msg_len), "allowed");
}

TEST_F(RingMemorySinkTest, CapacityOneEdgeCase)
{
  br_logger::RingMemorySink sink(1);

  sink.Write(make_test_entry(br_logger::LogLevel::INFO, "first"));
  EXPECT_EQ(sink.Size(), 1u);
  EXPECT_EQ(std::string(sink.At(0).msg, sink.At(0).msg_len), "first");

  sink.Write(make_test_entry(br_logger::LogLevel::INFO, "second"));
  EXPECT_EQ(sink.Size(), 1u);
  EXPECT_EQ(std::string(sink.At(0).msg, sink.At(0).msg_len), "second");
}

TEST_F(RingMemorySinkTest, WriteExactlyCapacityNoWrap)
{
  br_logger::RingMemorySink sink(4);

  for (uint64_t i = 0; i < 4; ++i)
  {
    auto msg = "item" + std::to_string(i);
    sink.Write(make_test_entry(br_logger::LogLevel::INFO, msg.c_str(), i));
  }

  EXPECT_EQ(sink.Size(), 4u);
  EXPECT_EQ(std::string(sink.At(0).msg, sink.At(0).msg_len), "item0");
  EXPECT_EQ(std::string(sink.At(1).msg, sink.At(1).msg_len), "item1");
  EXPECT_EQ(std::string(sink.At(2).msg, sink.At(2).msg_len), "item2");
  EXPECT_EQ(std::string(sink.At(3).msg, sink.At(3).msg_len), "item3");
}

TEST_F(RingMemorySinkTest, FlushDoesNotCrash)
{
  br_logger::RingMemorySink sink(10);
  EXPECT_NO_THROW(sink.Flush());
}

TEST_F(RingMemorySinkTest, DefaultLevelIsTrace)
{
  br_logger::RingMemorySink sink;
  EXPECT_EQ(sink.Level(), br_logger::LogLevel::TRACE);
}

TEST_F(RingMemorySinkTest, DefaultCapacityIs1024)
{
  br_logger::RingMemorySink sink;
  for (int i = 0; i < 1024; ++i)
  {
    sink.Write(make_test_entry());
  }
  EXPECT_EQ(sink.Size(), 1024u);
}

TEST_F(RingMemorySinkTest, DumpToFileInvalidPathReturnsFalse)
{
  br_logger::RingMemorySink sink(10);
  sink.Write(make_test_entry());
  EXPECT_FALSE(sink.DumpToFile("/nonexistent/path/dump.log"));
}

TEST_F(RingMemorySinkTest, DumpAfterWrapContainsOnlyNewest)
{
  br_logger::RingMemorySink sink(3);

  for (uint64_t i = 0; i < 5; ++i)
  {
    auto msg = "line" + std::to_string(i);
    sink.Write(make_test_entry(br_logger::LogLevel::INFO, msg.c_str(), i));
  }

  EXPECT_TRUE(sink.DumpToFile(tmp_file_.c_str()));
  std::string content = ReadFile(tmp_file_);

  EXPECT_EQ(content.find("line0"), std::string::npos);
  EXPECT_EQ(content.find("line1"), std::string::npos);
  EXPECT_NE(content.find("line2"), std::string::npos);
  EXPECT_NE(content.find("line3"), std::string::npos);
  EXPECT_NE(content.find("line4"), std::string::npos);
}
