#include "../include/br_logger/logger.hpp"
#include "../include/br_logger/sinks/callback_sink.hpp"
#include <gtest/gtest.h>
#include <vector>
#include <string>
#include <cstring>
#include <mutex>

class LoggerIntegrationTest : public ::testing::Test {
protected:
    static std::vector<br_logger::LogEntry> captured_;
    static std::mutex captured_mutex_;
    static bool setup_done_;

    static void SetUpTestSuite() {
        if (!setup_done_) {
            auto& logger = br_logger::Logger::instance();
            auto cb = std::make_unique<br_logger::CallbackSink>(
                [](const br_logger::LogEntry& entry) {
                    std::lock_guard<std::mutex> lock(captured_mutex_);
                    captured_.push_back(entry);
                });
            logger.add_sink(std::move(cb));
            logger.set_level(br_logger::LogLevel::Trace);
            setup_done_ = true;
        }
    }

    void SetUp() override {
        std::lock_guard<std::mutex> lock(captured_mutex_);
        captured_.clear();
        br_logger::Logger::instance().set_level(br_logger::LogLevel::Trace);
    }

    void drain_all() {
        br_logger::Logger::instance().drain(1024);
    }
};

std::vector<br_logger::LogEntry> LoggerIntegrationTest::captured_;
std::mutex LoggerIntegrationTest::captured_mutex_;
bool LoggerIntegrationTest::setup_done_ = false;

TEST_F(LoggerIntegrationTest, LogInfoBasic) {
    LOG_INFO("hello %s", "world");
    drain_all();

    ASSERT_EQ(captured_.size(), 1u);
    EXPECT_STREQ(captured_[0].msg, "hello world");
    EXPECT_EQ(captured_[0].level, br_logger::LogLevel::Info);
}

TEST_F(LoggerIntegrationTest, LogLevelFiltering) {
    br_logger::Logger::instance().set_level(br_logger::LogLevel::Warn);

    LOG_INFO("should not appear");
    drain_all();
    EXPECT_EQ(captured_.size(), 0u);

    LOG_WARN("should appear");
    drain_all();
    ASSERT_EQ(captured_.size(), 1u);
    EXPECT_EQ(captured_[0].level, br_logger::LogLevel::Warn);

    br_logger::Logger::instance().set_level(br_logger::LogLevel::Trace);
    captured_.clear();

    LOG_TRACE("trace visible");
    drain_all();
    ASSERT_EQ(captured_.size(), 1u);
    EXPECT_STREQ(captured_[0].msg, "trace visible");
}

TEST_F(LoggerIntegrationTest, DropCountReset) {
    auto& logger = br_logger::Logger::instance();
    logger.reset_drop_count();
    EXPECT_EQ(logger.drop_count(), 0u);
}

TEST_F(LoggerIntegrationTest, SequenceIdIncrement) {
    LOG_INFO("seq1");
    LOG_INFO("seq2");
    LOG_INFO("seq3");
    drain_all();

    ASSERT_EQ(captured_.size(), 3u);
    EXPECT_LT(captured_[0].sequence_id, captured_[1].sequence_id);
    EXPECT_LT(captured_[1].sequence_id, captured_[2].sequence_id);
}

TEST_F(LoggerIntegrationTest, SourceLocation) {
    LOG_INFO("location test");
    drain_all();

    ASSERT_EQ(captured_.size(), 1u);
    EXPECT_NE(captured_[0].file_name, nullptr);
    EXPECT_NE(captured_[0].function_name, nullptr);
    EXPECT_GT(captured_[0].line, 0u);

    std::string fname(captured_[0].file_name);
    EXPECT_NE(fname.find("test_logger_integration"), std::string::npos);
}

TEST_F(LoggerIntegrationTest, LogWarn) {
    LOG_WARN("warning %d", 42);
    drain_all();

    ASSERT_EQ(captured_.size(), 1u);
    EXPECT_EQ(captured_[0].level, br_logger::LogLevel::Warn);
    EXPECT_STREQ(captured_[0].msg, "warning 42");
}

TEST_F(LoggerIntegrationTest, LogError) {
    LOG_ERROR("error occurred: %s", "timeout");
    drain_all();

    ASSERT_EQ(captured_.size(), 1u);
    EXPECT_EQ(captured_[0].level, br_logger::LogLevel::Error);
    EXPECT_STREQ(captured_[0].msg, "error occurred: timeout");
}

TEST_F(LoggerIntegrationTest, ConditionalLog) {
    LOG_INFO_IF(false, "should not appear");
    drain_all();
    EXPECT_EQ(captured_.size(), 0u);

    LOG_INFO_IF(true, "should appear");
    drain_all();
    ASSERT_EQ(captured_.size(), 1u);
    EXPECT_STREQ(captured_[0].msg, "should appear");
}

TEST_F(LoggerIntegrationTest, LogOnce) {
    for (int i = 0; i < 5; ++i) {
        LOG_ONCE(Info, "only once");
    }
    drain_all();

    EXPECT_EQ(captured_.size(), 1u);
}

TEST_F(LoggerIntegrationTest, DrainManual) {
    LOG_INFO("drain test");
    size_t drained = br_logger::Logger::instance().drain(1024);
    EXPECT_GE(drained, 1u);
    ASSERT_GE(captured_.size(), 1u);
    EXPECT_STREQ(captured_[0].msg, "drain test");
}

TEST_F(LoggerIntegrationTest, LogTrace) {
    LOG_TRACE("trace msg");
    drain_all();

    ASSERT_EQ(captured_.size(), 1u);
    EXPECT_EQ(captured_[0].level, br_logger::LogLevel::Trace);
    EXPECT_STREQ(captured_[0].msg, "trace msg");
}

TEST_F(LoggerIntegrationTest, LogDebug) {
    LOG_DEBUG("debug msg %d", 99);
    drain_all();

    ASSERT_EQ(captured_.size(), 1u);
    EXPECT_EQ(captured_[0].level, br_logger::LogLevel::Debug);
    EXPECT_STREQ(captured_[0].msg, "debug msg 99");
}

TEST_F(LoggerIntegrationTest, LogFatal) {
    LOG_FATAL("fatal error");
    drain_all();

    ASSERT_EQ(captured_.size(), 1u);
    EXPECT_EQ(captured_[0].level, br_logger::LogLevel::Fatal);
    EXPECT_STREQ(captured_[0].msg, "fatal error");
}

TEST_F(LoggerIntegrationTest, TimestampsPopulated) {
    LOG_INFO("timestamp check");
    drain_all();

    ASSERT_EQ(captured_.size(), 1u);
    EXPECT_GT(captured_[0].timestamp_ns, 0u);
    EXPECT_GT(captured_[0].wall_clock_ns, 0u);
}

TEST_F(LoggerIntegrationTest, ThreadInfoPopulated) {
    LOG_INFO("thread check");
    drain_all();

    ASSERT_EQ(captured_.size(), 1u);
    EXPECT_GT(captured_[0].thread_id, 0u);
    EXPECT_GT(captured_[0].process_id, 0u);
}

TEST_F(LoggerIntegrationTest, ConditionalWarn) {
    LOG_WARN_IF(false, "no warn");
    drain_all();
    EXPECT_EQ(captured_.size(), 0u);

    LOG_WARN_IF(true, "yes warn");
    drain_all();
    ASSERT_EQ(captured_.size(), 1u);
    EXPECT_EQ(captured_[0].level, br_logger::LogLevel::Warn);
}

TEST_F(LoggerIntegrationTest, ConditionalError) {
    LOG_ERROR_IF(false, "no error");
    drain_all();
    EXPECT_EQ(captured_.size(), 0u);

    LOG_ERROR_IF(true, "yes error");
    drain_all();
    ASSERT_EQ(captured_.size(), 1u);
    EXPECT_EQ(captured_[0].level, br_logger::LogLevel::Error);
}

TEST_F(LoggerIntegrationTest, NoArgsFormat) {
    LOG_INFO("plain message no args");
    drain_all();

    ASSERT_EQ(captured_.size(), 1u);
    EXPECT_STREQ(captured_[0].msg, "plain message no args");
}

TEST_F(LoggerIntegrationTest, MultipleArgsFormat) {
    LOG_INFO("a=%d b=%s c=%.1f", 1, "two", 3.0);
    drain_all();

    ASSERT_EQ(captured_.size(), 1u);
    EXPECT_STREQ(captured_[0].msg, "a=1 b=two c=3.0");
}
