#include <gtest/gtest.h>

#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "../include/br_logger/formatters/pattern_formatter.hpp"
#include "../include/br_logger/log_entry.hpp"
#include "../include/br_logger/log_level.hpp"
#include "../include/br_logger/platform.hpp"
#include "../include/br_logger/sinks/console_sink.hpp"

#if defined(BR_LOG_PLATFORM_LINUX) || defined(BR_LOG_PLATFORM_MACOS)
#include <unistd.h>
#endif

static br_logger::LogEntry make_test_entry(
    br_logger::LogLevel level = br_logger::LogLevel::INFO)
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
  const char* msg = "test message";
  entry.msg_len = static_cast<uint16_t>(std::strlen(msg));
  std::strncpy(entry.msg, msg, BR_LOG_MAX_MSG_LEN);
  return entry;
}

class MockFormatter : public br_logger::IFormatter
{
 public:
  std::string last_formatted;

  size_t Format(const br_logger::LogEntry& entry, char* buf, size_t buf_size) override
  {
    last_formatted = std::string(entry.msg, entry.msg_len);
    size_t len = last_formatted.size();
    if (len >= buf_size)
    {
      len = buf_size - 1;
    }
    std::memcpy(buf, last_formatted.c_str(), len);
    buf[len] = '\0';
    return len;
  }
};

TEST(ConsoleSink, DefaultConstruction)
{
  br_logger::ConsoleSink sink;
  EXPECT_EQ(sink.Level(), br_logger::LogLevel::TRACE);
}

TEST(ConsoleSink, ForceColorTrue)
{
  br_logger::ConsoleSink sink(true);
  EXPECT_EQ(sink.Level(), br_logger::LogLevel::TRACE);
}

TEST(ConsoleSink, ForceColorFalse)
{
  br_logger::ConsoleSink sink(false);
  EXPECT_EQ(sink.Level(), br_logger::LogLevel::TRACE);
}

TEST(ConsoleSink, ForceColorNullopt)
{
  br_logger::ConsoleSink sink(std::nullopt);
  EXPECT_EQ(sink.Level(), br_logger::LogLevel::TRACE);
}

TEST(ConsoleSink, SetLevel)
{
  br_logger::ConsoleSink sink(false);
  sink.SetLevel(br_logger::LogLevel::WARN);
  EXPECT_EQ(sink.Level(), br_logger::LogLevel::WARN);
}

TEST(ConsoleSink, ShouldLogFiltering)
{
  br_logger::ConsoleSink sink(false);
  sink.SetLevel(br_logger::LogLevel::WARN);

  EXPECT_FALSE(sink.ShouldLog(br_logger::LogLevel::TRACE));
  EXPECT_FALSE(sink.ShouldLog(br_logger::LogLevel::DEBUG));
  EXPECT_FALSE(sink.ShouldLog(br_logger::LogLevel::INFO));
  EXPECT_TRUE(sink.ShouldLog(br_logger::LogLevel::WARN));
  EXPECT_TRUE(sink.ShouldLog(br_logger::LogLevel::ERROR));
  EXPECT_TRUE(sink.ShouldLog(br_logger::LogLevel::FATAL));
}

TEST(ConsoleSink, WriteFilteredByLevel)
{
  br_logger::ConsoleSink sink(false);
  sink.SetLevel(br_logger::LogLevel::WARN);

  auto entry = make_test_entry(br_logger::LogLevel::INFO);
  sink.Write(entry);
}

TEST(ConsoleSink, FlushDoesNotCrash)
{
  br_logger::ConsoleSink sink(false);
  EXPECT_NO_THROW(sink.Flush());
}

TEST(ConsoleSink, SetFormatterCustom)
{
  br_logger::ConsoleSink sink(false);
  auto mock = std::make_unique<MockFormatter>();
  auto* mock_ptr = mock.get();
  sink.SetFormatter(std::move(mock));

  auto entry = make_test_entry(br_logger::LogLevel::INFO);
  sink.Write(entry);
  EXPECT_EQ(mock_ptr->last_formatted, "test message");
}

TEST(ConsoleSink, DefaultFormatterCreatedOnFirstWrite)
{
  br_logger::ConsoleSink sink(false);
  auto entry = make_test_entry(br_logger::LogLevel::INFO);
  EXPECT_NO_THROW(sink.Write(entry));
}

static std::string capture_fd_output(int fd, const std::function<void()>& action)
{
  int pipefd[2];
  EXPECT_EQ(pipe(pipefd), 0);

  int saved_fd = dup(fd);
  EXPECT_NE(saved_fd, -1);

  dup2(pipefd[1], fd);
  close(pipefd[1]);

  action();
  fflush(nullptr);

  dup2(saved_fd, fd);
  close(saved_fd);

  char buf[4096] = {};
  ssize_t n = read(pipefd[0], buf, sizeof(buf) - 1);
  close(pipefd[0]);

  if (n > 0)
  {
    buf[n] = '\0';
    return std::string(buf, static_cast<size_t>(n));
  }
  return "";
}

TEST(ConsoleSink, WriteInfoToStdout)
{
  br_logger::ConsoleSink sink(false);
  auto entry = make_test_entry(br_logger::LogLevel::INFO);

  std::string stdout_output =
      capture_fd_output(STDOUT_FILENO, [&]() { sink.Write(entry); });

  EXPECT_FALSE(stdout_output.empty());
  EXPECT_NE(stdout_output.find("test message"), std::string::npos);
}

TEST(ConsoleSink, WriteTraceToStdout)
{
  br_logger::ConsoleSink sink(false);
  auto entry = make_test_entry(br_logger::LogLevel::TRACE);

  std::string stdout_output =
      capture_fd_output(STDOUT_FILENO, [&]() { sink.Write(entry); });

  EXPECT_FALSE(stdout_output.empty());
  EXPECT_NE(stdout_output.find("test message"), std::string::npos);
}

TEST(ConsoleSink, WriteDebugToStdout)
{
  br_logger::ConsoleSink sink(false);
  auto entry = make_test_entry(br_logger::LogLevel::DEBUG);

  std::string stdout_output =
      capture_fd_output(STDOUT_FILENO, [&]() { sink.Write(entry); });

  EXPECT_FALSE(stdout_output.empty());
}

TEST(ConsoleSink, WriteWarnToStderr)
{
  br_logger::ConsoleSink sink(false);
  auto entry = make_test_entry(br_logger::LogLevel::WARN);

  std::string stderr_output =
      capture_fd_output(STDERR_FILENO, [&]() { sink.Write(entry); });

  EXPECT_FALSE(stderr_output.empty());
  EXPECT_NE(stderr_output.find("test message"), std::string::npos);
}

TEST(ConsoleSink, WriteErrorToStderr)
{
  br_logger::ConsoleSink sink(false);
  auto entry = make_test_entry(br_logger::LogLevel::ERROR);

  std::string stderr_output =
      capture_fd_output(STDERR_FILENO, [&]() { sink.Write(entry); });

  EXPECT_FALSE(stderr_output.empty());
  EXPECT_NE(stderr_output.find("test message"), std::string::npos);
}

TEST(ConsoleSink, WriteFatalToStderr)
{
  br_logger::ConsoleSink sink(false);
  auto entry = make_test_entry(br_logger::LogLevel::FATAL);

  std::string stderr_output =
      capture_fd_output(STDERR_FILENO, [&]() { sink.Write(entry); });

  EXPECT_FALSE(stderr_output.empty());
  EXPECT_NE(stderr_output.find("test message"), std::string::npos);
}

TEST(ConsoleSink, WarnNotOnStdout)
{
  br_logger::ConsoleSink sink(false);
  auto entry = make_test_entry(br_logger::LogLevel::WARN);

  std::string stdout_output =
      capture_fd_output(STDOUT_FILENO, [&]() { sink.Write(entry); });

  EXPECT_TRUE(stdout_output.empty());
}

TEST(ConsoleSink, InfoNotOnStderr)
{
  br_logger::ConsoleSink sink(false);
  auto entry = make_test_entry(br_logger::LogLevel::INFO);

  std::string stderr_output =
      capture_fd_output(STDERR_FILENO, [&]() { sink.Write(entry); });

  EXPECT_TRUE(stderr_output.empty());
}

TEST(ConsoleSink, ColorEnabledOutput)
{
  br_logger::ConsoleSink sink(true);
  auto entry = make_test_entry(br_logger::LogLevel::INFO);

  std::string stdout_output =
      capture_fd_output(STDOUT_FILENO, [&]() { sink.Write(entry); });

  EXPECT_NE(stdout_output.find("\033["), std::string::npos);
}

TEST(ConsoleSink, ColorDisabledOutput)
{
  br_logger::ConsoleSink sink(false);
  auto entry = make_test_entry(br_logger::LogLevel::INFO);

  std::string stdout_output =
      capture_fd_output(STDOUT_FILENO, [&]() { sink.Write(entry); });

  EXPECT_EQ(stdout_output.find("\033["), std::string::npos);
}
