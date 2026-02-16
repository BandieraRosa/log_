#include <dirent.h>
#include <fcntl.h>
#include <gtest/gtest.h>
#include <sys/stat.h>
#include <unistd.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <memory>
#include <sstream>
#include <string>

#include "../include/br_logger/formatters/pattern_formatter.hpp"
#include "../include/br_logger/log_entry.hpp"
#include "../include/br_logger/log_level.hpp"
#include "../include/br_logger/sinks/rotating_file_sink.hpp"

static br_logger::LogEntry make_entry(
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

class MockFileFmt : public br_logger::IFormatter
{
 public:
  size_t Format(const br_logger::LogEntry& entry, char* buf, size_t buf_size) override
  {
    size_t len = entry.msg_len;
    if (len >= buf_size) len = buf_size - 1;
    std::memcpy(buf, entry.msg, len);
    buf[len] = '\0';
    return len;
  }
};

class RotatingFileSinkTest : public ::testing::Test
{
 protected:
  std::string tmp_dir_;
  std::string base_path_;

  void SetUp() override
  {
    char tmpl[] = "/tmp/br_logger_test_XXXXXX";
    char* dir = ::mkdtemp(tmpl);
    ASSERT_NE(dir, nullptr);
    tmp_dir_ = dir;
    base_path_ = tmp_dir_ + "/app.log";
  }

  void TearDown() override { remove_dir_recursive(tmp_dir_); }

  static std::string read_file(const std::string& path)
  {
    std::ifstream ifs(path);
    if (!ifs) return "";
    std::ostringstream ss;
    ss << ifs.rdbuf();
    return ss.str();
  }

  static bool file_exists(const std::string& path)
  {
    struct stat st{};
    return ::stat(path.c_str(), &st) == 0;
  }

  static size_t file_size(const std::string& path)
  {
    struct stat st{};
    if (::stat(path.c_str(), &st) != 0) return 0;
    return static_cast<size_t>(st.st_size);
  }

  static void remove_dir_recursive(const std::string& path)
  {
    DIR* d = ::opendir(path.c_str());
    if (!d) return;
    struct dirent* ent;
    while ((ent = ::readdir(d)) != nullptr)
    {
      std::string name = ent->d_name;
      if (name == "." || name == "..") continue;
      std::string full = path + "/" + name;
      struct stat st{};
      if (::stat(full.c_str(), &st) == 0 && S_ISDIR(st.st_mode))
      {
        remove_dir_recursive(full);
      }
      else
      {
        std::remove(full.c_str());
      }
    }
    ::closedir(d);
    ::rmdir(path.c_str());
  }
};

TEST_F(RotatingFileSinkTest, FileCreatedOnConstruction)
{
  br_logger::RotatingFileSink sink(base_path_, 1024, 3);
  EXPECT_TRUE(file_exists(base_path_));
}

TEST_F(RotatingFileSinkTest, WriteCreatesContent)
{
  br_logger::RotatingFileSink sink(base_path_, 4096, 3);
  auto fmt = std::make_unique<MockFileFmt>();
  sink.SetFormatter(std::move(fmt));

  sink.Write(make_entry());
  sink.Flush();

  std::string content = read_file(base_path_);
  EXPECT_NE(content.find("test message"), std::string::npos);
}

TEST_F(RotatingFileSinkTest, MultipleWritesAccumulate)
{
  br_logger::RotatingFileSink sink(base_path_, 4096, 3);
  auto fmt = std::make_unique<MockFileFmt>();
  sink.SetFormatter(std::move(fmt));

  sink.Write(make_entry(br_logger::LogLevel::Info, "line1"));
  sink.Write(make_entry(br_logger::LogLevel::Info, "line2"));
  sink.Write(make_entry(br_logger::LogLevel::Info, "line3"));
  sink.Flush();

  std::string content = read_file(base_path_);
  EXPECT_NE(content.find("line1"), std::string::npos);
  EXPECT_NE(content.find("line2"), std::string::npos);
  EXPECT_NE(content.find("line3"), std::string::npos);
}

TEST_F(RotatingFileSinkTest, RotationTriggersOnSizeExceeded)
{
  br_logger::RotatingFileSink sink(base_path_, 50, 3);
  auto fmt = std::make_unique<MockFileFmt>();
  sink.SetFormatter(std::move(fmt));

  for (int i = 0; i < 10; ++i)
  {
    std::string msg = "msg_" + std::to_string(i) + "_padding_data";
    sink.Write(make_entry(br_logger::LogLevel::Info, msg.c_str()));
  }
  sink.Flush();

  EXPECT_TRUE(file_exists(base_path_));
  EXPECT_TRUE(file_exists(base_path_ + ".1.log"));
}

TEST_F(RotatingFileSinkTest, RotatedFilesHaveCorrectNames)
{
  br_logger::RotatingFileSink sink(base_path_, 30, 5);
  auto fmt = std::make_unique<MockFileFmt>();
  sink.SetFormatter(std::move(fmt));

  for (int i = 0; i < 20; ++i)
  {
    std::string msg = "message_number_" + std::to_string(i);
    sink.Write(make_entry(br_logger::LogLevel::Info, msg.c_str()));
  }
  sink.Flush();

  EXPECT_TRUE(file_exists(base_path_));
  EXPECT_TRUE(file_exists(base_path_ + ".1.log"));
  EXPECT_TRUE(file_exists(base_path_ + ".2.log"));
}

TEST_F(RotatingFileSinkTest, OldFilesBeyondMaxFilesDeleted)
{
  const size_t max_files = 2;
  br_logger::RotatingFileSink sink(base_path_, 30, max_files);
  auto fmt = std::make_unique<MockFileFmt>();
  sink.SetFormatter(std::move(fmt));

  for (int i = 0; i < 50; ++i)
  {
    std::string msg = "padding_msg_" + std::to_string(i);
    sink.Write(make_entry(br_logger::LogLevel::Info, msg.c_str()));
  }
  sink.Flush();

  EXPECT_TRUE(file_exists(base_path_));
  EXPECT_TRUE(file_exists(base_path_ + ".1.log"));
  EXPECT_TRUE(file_exists(base_path_ + ".2.log"));
  EXPECT_FALSE(file_exists(base_path_ + ".3.log"));
}

TEST_F(RotatingFileSinkTest, FlushMakesContentPersistent)
{
  br_logger::RotatingFileSink sink(base_path_, 4096, 3);
  auto fmt = std::make_unique<MockFileFmt>();
  sink.SetFormatter(std::move(fmt));

  sink.Write(make_entry(br_logger::LogLevel::Info, "persistent data"));
  sink.Flush();

  EXPECT_GT(file_size(base_path_), 0u);
  std::string content = read_file(base_path_);
  EXPECT_NE(content.find("persistent data"), std::string::npos);
}

TEST_F(RotatingFileSinkTest, SetLevelFiltering)
{
  br_logger::RotatingFileSink sink(base_path_, 4096, 3);
  auto fmt = std::make_unique<MockFileFmt>();
  sink.SetFormatter(std::move(fmt));
  sink.SetLevel(br_logger::LogLevel::Warn);

  sink.Write(make_entry(br_logger::LogLevel::Info, "should_not_appear"));
  sink.Write(make_entry(br_logger::LogLevel::Debug, "also_not"));
  sink.Write(make_entry(br_logger::LogLevel::Warn, "warning_msg"));
  sink.Write(make_entry(br_logger::LogLevel::Error, "error_msg"));
  sink.Flush();

  std::string content = read_file(base_path_);
  EXPECT_EQ(content.find("should_not_appear"), std::string::npos);
  EXPECT_EQ(content.find("also_not"), std::string::npos);
  EXPECT_NE(content.find("warning_msg"), std::string::npos);
  EXPECT_NE(content.find("error_msg"), std::string::npos);
}

TEST_F(RotatingFileSinkTest, DefaultLevelIsTrace)
{
  br_logger::RotatingFileSink sink(base_path_, 4096, 3);
  EXPECT_EQ(sink.Level(), br_logger::LogLevel::Trace);
}

TEST_F(RotatingFileSinkTest, DefaultFormatterCreatedOnWrite)
{
  br_logger::RotatingFileSink sink(base_path_, 4096, 3);
  sink.Write(make_entry());
  sink.Flush();

  std::string content = read_file(base_path_);
  EXPECT_FALSE(content.empty());
  EXPECT_NE(content.find("test message"), std::string::npos);
}

TEST_F(RotatingFileSinkTest, FileReopenedAfterRotation)
{
  br_logger::RotatingFileSink sink(base_path_, 40, 3);
  auto fmt = std::make_unique<MockFileFmt>();
  sink.SetFormatter(std::move(fmt));

  for (int i = 0; i < 5; ++i)
  {
    std::string msg = "after_rotate_" + std::to_string(i);
    sink.Write(make_entry(br_logger::LogLevel::Info, msg.c_str()));
  }
  sink.Flush();

  EXPECT_TRUE(file_exists(base_path_));
  std::string content = read_file(base_path_);
  EXPECT_FALSE(content.empty());
}

TEST_F(RotatingFileSinkTest, RotatedFileContainsPreviousContent)
{
  br_logger::RotatingFileSink sink(base_path_, 50, 3);
  auto fmt = std::make_unique<MockFileFmt>();
  sink.SetFormatter(std::move(fmt));

  sink.Write(make_entry(br_logger::LogLevel::Info, "first_batch_data_xyz"));
  sink.Flush();

  for (int i = 0; i < 10; ++i)
  {
    std::string msg = "second_batch_" + std::to_string(i) + "_pad";
    sink.Write(make_entry(br_logger::LogLevel::Info, msg.c_str()));
  }
  sink.Flush();

  std::string rotated = base_path_ + ".1.log";
  if (file_exists(rotated))
  {
    std::string content = read_file(rotated);
    EXPECT_FALSE(content.empty());
  }
}

TEST_F(RotatingFileSinkTest, DestructorClosesCleanly)
{
  {
    br_logger::RotatingFileSink sink(base_path_, 4096, 3);
    auto fmt = std::make_unique<MockFileFmt>();
    sink.SetFormatter(std::move(fmt));
    sink.Write(make_entry(br_logger::LogLevel::Info, "before_destruct"));
  }

  std::string content = read_file(base_path_);
  EXPECT_NE(content.find("before_destruct"), std::string::npos);
}

TEST_F(RotatingFileSinkTest, MaxFilesOne)
{
  br_logger::RotatingFileSink sink(base_path_, 30, 1);
  auto fmt = std::make_unique<MockFileFmt>();
  sink.SetFormatter(std::move(fmt));

  for (int i = 0; i < 20; ++i)
  {
    std::string msg = "maxone_msg_" + std::to_string(i);
    sink.Write(make_entry(br_logger::LogLevel::Info, msg.c_str()));
  }
  sink.Flush();

  EXPECT_TRUE(file_exists(base_path_));
  EXPECT_TRUE(file_exists(base_path_ + ".1.log"));
  EXPECT_FALSE(file_exists(base_path_ + ".2.log"));
}

TEST_F(RotatingFileSinkTest, FlushDoesNotCrashOnEmptyFile)
{
  br_logger::RotatingFileSink sink(base_path_, 4096, 3);
  EXPECT_NO_THROW(sink.Flush());
}

TEST_F(RotatingFileSinkTest, ShouldLogFiltering)
{
  br_logger::RotatingFileSink sink(base_path_, 4096, 3);
  sink.SetLevel(br_logger::LogLevel::Error);

  EXPECT_FALSE(sink.ShouldLog(br_logger::LogLevel::Trace));
  EXPECT_FALSE(sink.ShouldLog(br_logger::LogLevel::Debug));
  EXPECT_FALSE(sink.ShouldLog(br_logger::LogLevel::Info));
  EXPECT_FALSE(sink.ShouldLog(br_logger::LogLevel::Warn));
  EXPECT_TRUE(sink.ShouldLog(br_logger::LogLevel::Error));
  EXPECT_TRUE(sink.ShouldLog(br_logger::LogLevel::Fatal));
}

TEST_F(RotatingFileSinkTest, ExistingFileAppendsOnReopen)
{
  {
    br_logger::RotatingFileSink sink(base_path_, 4096, 3);
    auto fmt = std::make_unique<MockFileFmt>();
    sink.SetFormatter(std::move(fmt));
    sink.Write(make_entry(br_logger::LogLevel::Info, "session_one"));
    sink.Flush();
  }

  {
    br_logger::RotatingFileSink sink(base_path_, 4096, 3);
    auto fmt = std::make_unique<MockFileFmt>();
    sink.SetFormatter(std::move(fmt));
    sink.Write(make_entry(br_logger::LogLevel::Info, "session_two"));
    sink.Flush();
  }

  std::string content = read_file(base_path_);
  EXPECT_NE(content.find("session_one"), std::string::npos);
  EXPECT_NE(content.find("session_two"), std::string::npos);
}
