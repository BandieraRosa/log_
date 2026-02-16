#include "br_logger/sinks/daily_file_sink.hpp"

#include <dirent.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#include <cstdio>
#include <cstring>
#include <ctime>
#include <string>

#include "br_logger/formatters/pattern_formatter.hpp"

namespace br_logger
{

void DailyFileSink::MkdirRecursive(const std::string& path)
{
  std::string tmp;
  for (size_t i = 0; i < path.size(); ++i)
  {
    tmp += path[i];
    if (path[i] == '/' || i == path.size() - 1)
    {
      ::mkdir(tmp.c_str(), 0755);
    }
  }
}

DailyFileSink::DailyFileSink(const std::string& base_dir, const std::string& base_name,
                             size_t max_days, bool use_utc)
    : base_dir_(base_dir),
      base_name_(base_name),
      max_days_(max_days),
      use_utc_(use_utc),
      fd_(-1),
      current_day_(-1)
{
  MkdirRecursive(base_dir_);
  OpenFileForToday();
}

DailyFileSink::~DailyFileSink()
{
  if (fd_ >= 0)
  {
    ::fsync(fd_);
    ::close(fd_);
    fd_ = -1;
  }
}

int DailyFileSink::GetDay(std::time_t t) const
{
  std::tm tm_buf{};
  if (use_utc_)
  {
    ::gmtime_r(&t, &tm_buf);
  }
  else
  {
    ::localtime_r(&t, &tm_buf);
  }
  return tm_buf.tm_yday + tm_buf.tm_year * 366;
}

std::string DailyFileSink::MakeFilename(std::time_t t) const
{
  std::tm tm_buf{};
  if (use_utc_)
  {
    ::gmtime_r(&t, &tm_buf);
  }
  else
  {
    ::localtime_r(&t, &tm_buf);
  }
  char date_str[32];
  std::snprintf(date_str, sizeof(date_str), "%04d-%02d-%02d", tm_buf.tm_year + 1900,
                tm_buf.tm_mon + 1, tm_buf.tm_mday);

  std::string result = base_dir_;
  if (!result.empty() && result.back() != '/')
  {
    result += '/';
  }
  result += base_name_;
  result += '_';
  result += date_str;
  result += ".log";
  return result;
}

void DailyFileSink::OpenFileForToday()
{
  std::time_t now = std::time(nullptr);
  std::string filename = MakeFilename(now);

  if (fd_ >= 0)
  {
    ::fsync(fd_);
    ::close(fd_);
  }

  fd_ = ::open(filename.c_str(), O_WRONLY | O_CREAT | O_APPEND, 0644);
  current_day_ = GetDay(now);

  if (max_days_ > 0)
  {
    CleanupOldFiles();
  }
}

void DailyFileSink::CleanupOldFiles()
{
  DIR* dir = ::opendir(base_dir_.c_str());
  if (!dir) return;

  std::string prefix = base_name_ + "_";
  std::string suffix = ".log";

  std::time_t now = std::time(nullptr);
  double max_seconds = static_cast<double>(max_days_) * 86400.0;

  struct dirent* ent;
  while ((ent = ::readdir(dir)) != nullptr)
  {
    std::string name(ent->d_name);
    if (name.size() < prefix.size() + suffix.size()) continue;
    if (name.compare(0, prefix.size(), prefix) != 0) continue;
    if (name.compare(name.size() - suffix.size(), suffix.size(), suffix) != 0) continue;

    std::string full_path = base_dir_;
    if (!full_path.empty() && full_path.back() != '/')
    {
      full_path += '/';
    }
    full_path += name;

    struct stat st{};
    if (::stat(full_path.c_str(), &st) == 0)
    {
      double age = std::difftime(now, st.st_mtime);
      if (age > max_seconds)
      {
        ::unlink(full_path.c_str());
      }
    }
  }
  ::closedir(dir);
}

void DailyFileSink::Write(const LogEntry& entry)
{
  if (!ShouldLog(entry.level))
  {
    return;
  }

  if (!formatter_)
  {
    formatter_ = std::make_unique<PatternFormatter>(
        "[%D %T%e] [%C%L%R] [tid:%t] [%f:%#::%n] %g %m", false);
  }

  std::time_t now = std::time(nullptr);
  int today = GetDay(now);
  if (today != current_day_)
  {
    OpenFileForToday();
  }

  size_t len = DoFormat(entry);
  if (len == 0 || fd_ < 0)
  {
    return;
  }

  ::write(fd_, format_buf_, len);
  ::write(fd_, "\n", 1);
}

void DailyFileSink::Flush()
{
  if (fd_ >= 0)
  {
    ::fsync(fd_);
  }
}

}  // namespace br_logger
