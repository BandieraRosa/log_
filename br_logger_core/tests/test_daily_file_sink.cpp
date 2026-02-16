#include <dirent.h>
#include <fcntl.h>
#include <gtest/gtest.h>
#include <sys/stat.h>
#include <unistd.h>

#include <cstdio>
#include <cstring>
#include <ctime>
#include <fstream>
#include <memory>
#include <string>

#include "../include/br_logger/formatters/pattern_formatter.hpp"
#include "../include/br_logger/log_entry.hpp"
#include "../include/br_logger/log_level.hpp"
#include "../include/br_logger/sinks/daily_file_sink.hpp"

static br_logger::LogEntry make_test_entry(
    br_logger::LogLevel level = br_logger::LogLevel::Info)
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
  const char* msg = "daily test message";
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
    if (len >= buf_size) len = buf_size - 1;
    std::memcpy(buf, last_formatted.c_str(), len);
    buf[len] = '\0';
    return len;
  }
};

static std::string read_file_contents(const std::string& path)
{
  int fd = ::open(path.c_str(), O_RDONLY);
  if (fd < 0) return "";
  char buf[8192] = {};
  ssize_t n = ::read(fd, buf, sizeof(buf) - 1);
  ::close(fd);
  if (n > 0)
  {
    buf[n] = '\0';
    return std::string(buf, static_cast<size_t>(n));
  }
  return "";
}

static void remove_directory_recursive(const std::string& path)
{
  DIR* dir = ::opendir(path.c_str());
  if (!dir) return;
  struct dirent* ent;
  while ((ent = ::readdir(dir)) != nullptr)
  {
    std::string name(ent->d_name);
    if (name == "." || name == "..") continue;
    std::string full = path + "/" + name;
    struct stat st{};
    if (::stat(full.c_str(), &st) == 0)
    {
      if (S_ISDIR(st.st_mode))
      {
        remove_directory_recursive(full);
      }
      else
      {
        ::unlink(full.c_str());
      }
    }
  }
  ::closedir(dir);
  ::rmdir(path.c_str());
}

class DailyFileSinkTest : public ::testing::Test
{
 protected:
  std::string test_dir_;

  void SetUp() override
  {
    char tmpl[] = "/tmp/br_logger_daily_test_XXXXXX";
    char* dir = ::mkdtemp(tmpl);
    ASSERT_NE(dir, nullptr);
    test_dir_ = dir;
  }

  void TearDown() override { remove_directory_recursive(test_dir_); }

  std::string expected_filename_for_today(const std::string& base_name)
  {
    std::time_t now = std::time(nullptr);
    std::tm tm_buf{};
    ::localtime_r(&now, &tm_buf);
    char date_str[16];
    std::snprintf(date_str, sizeof(date_str), "%04d-%02d-%02d", tm_buf.tm_year + 1900,
                  tm_buf.tm_mon + 1, tm_buf.tm_mday);
    return test_dir_ + "/" + base_name + "_" + date_str + ".log";
  }
};

TEST_F(DailyFileSinkTest, FileCreatedWithCorrectName)
{
  br_logger::DailyFileSink sink(test_dir_, "app");
  std::string expected = expected_filename_for_today("app");
  struct stat st{};
  EXPECT_EQ(::stat(expected.c_str(), &st), 0);
}

TEST_F(DailyFileSinkTest, WriteCreatesContent)
{
  {
    br_logger::DailyFileSink sink(test_dir_, "app");
    auto entry = make_test_entry();
    sink.Write(entry);
    sink.Flush();
  }

  std::string expected = expected_filename_for_today("app");
  std::string content = read_file_contents(expected);
  EXPECT_FALSE(content.empty());
  EXPECT_NE(content.find("daily test message"), std::string::npos);
}

TEST_F(DailyFileSinkTest, FilenameFormatMatchesPattern)
{
  br_logger::DailyFileSink sink(test_dir_, "myapp");
  std::time_t t = 1739692200;
  std::string filename = sink.MakeFilename(t);

  EXPECT_NE(filename.find("myapp_"), std::string::npos);
  EXPECT_NE(filename.find(".log"), std::string::npos);

  size_t underscore = filename.rfind("myapp_");
  std::string date_part = filename.substr(underscore + 6, 10);
  EXPECT_EQ(date_part[4], '-');
  EXPECT_EQ(date_part[7], '-');
}

TEST_F(DailyFileSinkTest, MakeFilenameWithSpecificTime)
{
  br_logger::DailyFileSink sink(test_dir_, "test", 0, true);

  std::tm tm_buf{};
  tm_buf.tm_year = 126;
  tm_buf.tm_mon = 1;
  tm_buf.tm_mday = 16;
  tm_buf.tm_hour = 12;
  std::time_t t = ::timegm(&tm_buf);

  std::string filename = sink.MakeFilename(t);
  EXPECT_NE(filename.find("test_2026-02-16.log"), std::string::npos);
}

TEST_F(DailyFileSinkTest, MakeFilenameUTCvsLocal)
{
  br_logger::DailyFileSink sink_utc(test_dir_, "u", 0, true);
  br_logger::DailyFileSink sink_local(test_dir_, "l", 0, false);

  std::time_t t = std::time(nullptr);
  std::string fn_utc = sink_utc.MakeFilename(t);
  std::string fn_local = sink_local.MakeFilename(t);

  EXPECT_NE(fn_utc.find(".log"), std::string::npos);
  EXPECT_NE(fn_local.find(".log"), std::string::npos);
}

TEST_F(DailyFileSinkTest, FlushWorks)
{
  br_logger::DailyFileSink sink(test_dir_, "app");
  auto entry = make_test_entry();
  sink.Write(entry);
  EXPECT_NO_THROW(sink.Flush());

  std::string expected = expected_filename_for_today("app");
  std::string content = read_file_contents(expected);
  EXPECT_NE(content.find("daily test message"), std::string::npos);
}

TEST_F(DailyFileSinkTest, SetLevelFiltering)
{
  br_logger::DailyFileSink sink(test_dir_, "app");
  sink.SetLevel(br_logger::LogLevel::Warn);

  auto info_entry = make_test_entry(br_logger::LogLevel::Info);
  sink.Write(info_entry);
  sink.Flush();

  std::string expected = expected_filename_for_today("app");
  std::string content = read_file_contents(expected);
  EXPECT_TRUE(content.empty());
}

TEST_F(DailyFileSinkTest, SetLevelAllowsHigherLevel)
{
  br_logger::DailyFileSink sink(test_dir_, "app");
  sink.SetLevel(br_logger::LogLevel::Warn);

  auto warn_entry = make_test_entry(br_logger::LogLevel::Warn);
  sink.Write(warn_entry);
  sink.Flush();

  std::string expected = expected_filename_for_today("app");
  std::string content = read_file_contents(expected);
  EXPECT_FALSE(content.empty());
}

TEST_F(DailyFileSinkTest, MultipleWritesToSameFile)
{
  {
    br_logger::DailyFileSink sink(test_dir_, "app");
    for (int i = 0; i < 5; ++i)
    {
      auto entry = make_test_entry();
      sink.Write(entry);
    }
    sink.Flush();
  }

  std::string expected = expected_filename_for_today("app");
  std::string content = read_file_contents(expected);

  size_t count = 0;
  size_t pos = 0;
  while ((pos = content.find("daily test message", pos)) != std::string::npos)
  {
    ++count;
    pos += 18;
  }
  EXPECT_EQ(count, 5u);
}

TEST_F(DailyFileSinkTest, ConstructorCreatesDirectoryIfNotExist)
{
  std::string nested_dir = test_dir_ + "/a/b/c";
  br_logger::DailyFileSink sink(nested_dir, "app");

  struct stat st{};
  EXPECT_EQ(::stat(nested_dir.c_str(), &st), 0);
  EXPECT_TRUE(S_ISDIR(st.st_mode));
}

TEST_F(DailyFileSinkTest, DefaultLevelIsTrace)
{
  br_logger::DailyFileSink sink(test_dir_, "app");
  EXPECT_EQ(sink.Level(), br_logger::LogLevel::Trace);
}

TEST_F(DailyFileSinkTest, CustomFormatterIsUsed)
{
  br_logger::DailyFileSink sink(test_dir_, "app");
  auto mock = std::make_unique<MockFormatter>();
  auto* mock_ptr = mock.get();
  sink.SetFormatter(std::move(mock));

  auto entry = make_test_entry();
  sink.Write(entry);
  EXPECT_EQ(mock_ptr->last_formatted, "daily test message");
}

TEST_F(DailyFileSinkTest, DefaultFormatterCreatedOnFirstWrite)
{
  br_logger::DailyFileSink sink(test_dir_, "app");
  auto entry = make_test_entry();
  EXPECT_NO_THROW(sink.Write(entry));
}

TEST_F(DailyFileSinkTest, MakeFilenameDifferentDays)
{
  br_logger::DailyFileSink sink(test_dir_, "log", 0, true);

  std::tm tm1{};
  tm1.tm_year = 126;
  tm1.tm_mon = 0;
  tm1.tm_mday = 1;
  tm1.tm_hour = 12;
  std::time_t t1 = ::timegm(&tm1);

  std::tm tm2{};
  tm2.tm_year = 126;
  tm2.tm_mon = 0;
  tm2.tm_mday = 2;
  tm2.tm_hour = 12;
  std::time_t t2 = ::timegm(&tm2);

  std::string fn1 = sink.MakeFilename(t1);
  std::string fn2 = sink.MakeFilename(t2);

  EXPECT_NE(fn1, fn2);
  EXPECT_NE(fn1.find("2026-01-01"), std::string::npos);
  EXPECT_NE(fn2.find("2026-01-02"), std::string::npos);
}

TEST_F(DailyFileSinkTest, AppendToExistingFile)
{
  std::string expected = expected_filename_for_today("app");

  {
    br_logger::DailyFileSink sink(test_dir_, "app");
    auto entry = make_test_entry();
    sink.Write(entry);
    sink.Flush();
  }

  {
    br_logger::DailyFileSink sink(test_dir_, "app");
    auto entry = make_test_entry();
    sink.Write(entry);
    sink.Flush();
  }

  std::string content = read_file_contents(expected);
  size_t count = 0;
  size_t pos = 0;
  while ((pos = content.find("daily test message", pos)) != std::string::npos)
  {
    ++count;
    pos += 18;
  }
  EXPECT_EQ(count, 2u);
}
