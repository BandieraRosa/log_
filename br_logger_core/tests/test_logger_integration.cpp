#include <gtest/gtest.h>

#include <cstring>
#include <mutex>
#include <string>
#include <vector>

#include "../include/br_logger/logger.hpp"
#include "../include/br_logger/sinks/callback_sink.hpp"

class LoggerIntegrationTest : public ::testing::Test
{
 protected:
  static std::vector<br_logger::LogEntry> captured_;
  static std::mutex captured_mutex_;
  static bool setup_done_;

  static void SetUpTestSuite()
  {
    if (!setup_done_)
    {
      auto& logger = br_logger::Logger::Instance();
      auto cb = std::make_unique<br_logger::CallbackSink>(
          [](const br_logger::LogEntry& entry)
          {
            std::lock_guard<std::mutex> lock(captured_mutex_);
            captured_.push_back(entry);
          });
      logger.AddSink(std::move(cb));
      logger.SetLevel(br_logger::LogLevel::TRACE);
      setup_done_ = true;
    }
  }

  void SetUp() override
  {
    std::lock_guard<std::mutex> lock(captured_mutex_);
    captured_.clear();
    br_logger::Logger::Instance().SetLevel(br_logger::LogLevel::TRACE);
  }

  void DrainAll() { br_logger::Logger::Instance().Drain(1024); }
};

std::vector<br_logger::LogEntry> LoggerIntegrationTest::captured_;
std::mutex LoggerIntegrationTest::captured_mutex_;
bool LoggerIntegrationTest::setup_done_ = false;

TEST_F(LoggerIntegrationTest, LogInfoBasic)
{
  LOG_INFO("hello %s", "world");
  DrainAll();

  ASSERT_EQ(captured_.size(), 1u);
  EXPECT_STREQ(captured_[0].msg, "hello world");
  EXPECT_EQ(captured_[0].level, br_logger::LogLevel::INFO);
}

TEST_F(LoggerIntegrationTest, LogLevelFiltering)
{
  br_logger::Logger::Instance().SetLevel(br_logger::LogLevel::WARN);

  LOG_INFO("should not appear");
  DrainAll();
  EXPECT_EQ(captured_.size(), 0u);

  LOG_WARN("should appear");
  DrainAll();
  ASSERT_EQ(captured_.size(), 1u);
  EXPECT_EQ(captured_[0].level, br_logger::LogLevel::WARN);

  br_logger::Logger::Instance().SetLevel(br_logger::LogLevel::TRACE);
  captured_.clear();

  LOG_TRACE("trace visible");
  DrainAll();
  ASSERT_EQ(captured_.size(), 1u);
  EXPECT_STREQ(captured_[0].msg, "trace visible");
}

TEST_F(LoggerIntegrationTest, DropCountReset)
{
  auto& logger = br_logger::Logger::Instance();
  logger.ResetDropCount();
  EXPECT_EQ(logger.DropCount(), 0u);
}

TEST_F(LoggerIntegrationTest, SequenceIdIncrement)
{
  LOG_INFO("seq1");
  LOG_INFO("seq2");
  LOG_INFO("seq3");
  DrainAll();

  ASSERT_EQ(captured_.size(), 3u);
  EXPECT_LT(captured_[0].sequence_id, captured_[1].sequence_id);
  EXPECT_LT(captured_[1].sequence_id, captured_[2].sequence_id);
}

TEST_F(LoggerIntegrationTest, SourceLocation)
{
  LOG_INFO("location test");
  DrainAll();

  ASSERT_EQ(captured_.size(), 1u);
  EXPECT_NE(captured_[0].file_name, nullptr);
  EXPECT_NE(captured_[0].function_name, nullptr);
  EXPECT_GT(captured_[0].line, 0u);

  std::string fname(captured_[0].file_name);
  EXPECT_NE(fname.find("test_logger_integration"), std::string::npos);
}

TEST_F(LoggerIntegrationTest, LogWarn)
{
  LOG_WARN("warning %d", 42);
  DrainAll();

  ASSERT_EQ(captured_.size(), 1u);
  EXPECT_EQ(captured_[0].level, br_logger::LogLevel::WARN);
  EXPECT_STREQ(captured_[0].msg, "warning 42");
}

TEST_F(LoggerIntegrationTest, LogError)
{
  LOG_ERROR("error occurred: %s", "timeout");
  DrainAll();

  ASSERT_EQ(captured_.size(), 1u);
  EXPECT_EQ(captured_[0].level, br_logger::LogLevel::ERROR);
  EXPECT_STREQ(captured_[0].msg, "error occurred: timeout");
}

TEST_F(LoggerIntegrationTest, ConditionalLog)
{
  LOG_INFO_IF(false, "should not appear");
  DrainAll();
  EXPECT_EQ(captured_.size(), 0u);

  LOG_INFO_IF(true, "should appear");
  DrainAll();
  ASSERT_EQ(captured_.size(), 1u);
  EXPECT_STREQ(captured_[0].msg, "should appear");
}

TEST_F(LoggerIntegrationTest, LogOnce)
{
  for (int i = 0; i < 5; ++i)
  {
    LOG_ONCE(INFO, "only once");
  }
  DrainAll();

  EXPECT_EQ(captured_.size(), 1u);
}

TEST_F(LoggerIntegrationTest, DrainManual)
{
  LOG_INFO("drain test");
  size_t drained = br_logger::Logger::Instance().Drain(1024);
  EXPECT_GE(drained, 1u);
  ASSERT_GE(captured_.size(), 1u);
  EXPECT_STREQ(captured_[0].msg, "drain test");
}

TEST_F(LoggerIntegrationTest, LogTrace)
{
  LOG_TRACE("trace msg");
  DrainAll();

  ASSERT_EQ(captured_.size(), 1u);
  EXPECT_EQ(captured_[0].level, br_logger::LogLevel::TRACE);
  EXPECT_STREQ(captured_[0].msg, "trace msg");
}

TEST_F(LoggerIntegrationTest, LogDebug)
{
  LOG_DEBUG("debug msg %d", 99);
  DrainAll();

  ASSERT_EQ(captured_.size(), 1u);
  EXPECT_EQ(captured_[0].level, br_logger::LogLevel::DEBUG);
  EXPECT_STREQ(captured_[0].msg, "debug msg 99");
}

TEST_F(LoggerIntegrationTest, LogFatal)
{
  LOG_FATAL("fatal error");
  DrainAll();

  ASSERT_EQ(captured_.size(), 1u);
  EXPECT_EQ(captured_[0].level, br_logger::LogLevel::FATAL);
  EXPECT_STREQ(captured_[0].msg, "fatal error");
}

TEST_F(LoggerIntegrationTest, TimestampsPopulated)
{
  LOG_INFO("timestamp check");
  DrainAll();

  ASSERT_EQ(captured_.size(), 1u);
  EXPECT_GT(captured_[0].timestamp_ns, 0u);
  EXPECT_GT(captured_[0].wall_clock_ns, 0u);
}

TEST_F(LoggerIntegrationTest, ThreadInfoPopulated)
{
  LOG_INFO("thread check");
  DrainAll();

  ASSERT_EQ(captured_.size(), 1u);
  EXPECT_GT(captured_[0].thread_id, 0u);
  EXPECT_GT(captured_[0].process_id, 0u);
}

TEST_F(LoggerIntegrationTest, ConditionalWarn)
{
  LOG_WARN_IF(false, "no warn");
  DrainAll();
  EXPECT_EQ(captured_.size(), 0u);

  LOG_WARN_IF(true, "yes warn");
  DrainAll();
  ASSERT_EQ(captured_.size(), 1u);
  EXPECT_EQ(captured_[0].level, br_logger::LogLevel::WARN);
}

TEST_F(LoggerIntegrationTest, ConditionalError)
{
  LOG_ERROR_IF(false, "no error");
  DrainAll();
  EXPECT_EQ(captured_.size(), 0u);

  LOG_ERROR_IF(true, "yes error");
  DrainAll();
  ASSERT_EQ(captured_.size(), 1u);
  EXPECT_EQ(captured_[0].level, br_logger::LogLevel::ERROR);
}

TEST_F(LoggerIntegrationTest, NoArgsFormat)
{
  LOG_INFO("plain message no args");
  DrainAll();

  ASSERT_EQ(captured_.size(), 1u);
  EXPECT_STREQ(captured_[0].msg, "plain message no args");
}

TEST_F(LoggerIntegrationTest, MultipleArgsFormat)
{
  LOG_INFO("a=%d b=%s c=%.1f", 1, "two", 3.0);
  DrainAll();

  ASSERT_EQ(captured_.size(), 1u);
  EXPECT_STREQ(captured_[0].msg, "a=1 b=two c=3.0");
}
