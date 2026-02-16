#include "br_logger/sinks/rotating_file_sink.hpp"

#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#include <cstdio>
#include <cstring>

#include "br_logger/formatters/pattern_formatter.hpp"

namespace br_logger
{

RotatingFileSink::RotatingFileSink(const std::string& base_path, size_t max_file_size,
                                   size_t max_files)
    : base_path_(base_path),
      max_file_size_(max_file_size),
      max_files_(max_files),
      current_size_(0),
      fd_(-1)
{
  OpenFile();
}

RotatingFileSink::~RotatingFileSink()
{
  if (fd_ >= 0)
  {
    ::fsync(fd_);
    ::close(fd_);
    fd_ = -1;
  }
}

void RotatingFileSink::OpenFile()
{
  fd_ = ::open(base_path_.c_str(), O_WRONLY | O_CREAT | O_APPEND, 0644);
  if (fd_ < 0)
  {
    std::fprintf(stderr, "RotatingFileSink: failed to open '%s': %s\n",
                 base_path_.c_str(), std::strerror(errno));
    return;
  }

  struct stat st{};
  if (::fstat(fd_, &st) == 0)
  {
    current_size_ = static_cast<size_t>(st.st_size);
  }
  else
  {
    current_size_ = 0;
  }
}

void RotatingFileSink::Rotate()
{
  if (fd_ >= 0)
  {
    ::close(fd_);
    fd_ = -1;
  }

  for (size_t i = max_files_; i > 0; --i)
  {
    std::string src =
        (i == 1) ? base_path_ : base_path_ + "." + std::to_string(i - 1) + ".log";
    std::string dst = base_path_ + "." + std::to_string(i) + ".log";

    if (i == max_files_)
    {
      std::remove(dst.c_str());
    }
    std::rename(src.c_str(), dst.c_str());
  }

  current_size_ = 0;
  OpenFile();
}

void RotatingFileSink::Write(const LogEntry& entry)
{
  if (!ShouldLog(entry.level))
  {
    return;
  }

  if (!formatter_)
  {
    formatter_ = std::make_unique<PatternFormatter>(
        "[%D %T%e] [%L] [tid:%t] [%f:%#::%n] %g %m", false);
  }

  size_t len = DoFormat(entry);
  if (len == 0 || fd_ < 0)
  {
    return;
  }

  if (current_size_ + len + 1 > max_file_size_)
  {
    Rotate();
    if (fd_ < 0)
    {
      return;
    }
  }

  ssize_t written = ::write(fd_, format_buf_, len);
  if (written > 0)
  {
    current_size_ += static_cast<size_t>(written);
  }
  written = ::write(fd_, "\n", 1);
  if (written > 0)
  {
    current_size_ += 1;
  }
}

void RotatingFileSink::Flush()
{
  if (fd_ >= 0)
  {
    ::fdatasync(fd_);
  }
}

}  // namespace br_logger
