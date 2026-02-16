#include "br_logger/sinks/ring_memory_sink.hpp"

#include <fcntl.h>
#include <unistd.h>

#include <cstring>

#include "br_logger/formatters/pattern_formatter.hpp"

namespace br_logger
{

RingMemorySink::RingMemorySink(size_t capacity) : capacity_(capacity), head_(0), count_(0)
{
  buffer_.resize(capacity);
}

void RingMemorySink::Write(const LogEntry& entry)
{
  if (!ShouldLog(entry.level))
  {
    return;
  }
  buffer_[head_] = entry;
  head_ = (head_ + 1) % capacity_;
  if (count_ < capacity_)
  {
    ++count_;
  }
}

void RingMemorySink::Flush() {}

bool RingMemorySink::DumpToFile(const char* path)
{
  int fd = ::open(path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
  if (fd < 0)
  {
    return false;
  }

  if (count_ == 0)
  {
    ::close(fd);
    return true;
  }

  PatternFormatter default_fmt("[%D %T%e] [%L] [tid:%t] %m", false);
  IFormatter* fmt = formatter_ ? formatter_.get() : &default_fmt;
  char buf[2048];

  for (size_t i = 0; i < count_; ++i)
  {
    const LogEntry& entry = At(i);
    size_t len = fmt->Format(entry, buf, sizeof(buf) - 1);
    if (len > 0)
    {
      buf[len] = '\n';
      ::write(fd, buf, len + 1);
    }
  }

  ::close(fd);
  return true;
}

size_t RingMemorySink::Size() const { return count_; }

const LogEntry& RingMemorySink::At(size_t index) const
{
  size_t start = 0;
  if (count_ < capacity_)
  {
    start = 0;
  }
  else
  {
    start = head_;
  }
  return buffer_[(start + index) % capacity_];
}

}  // namespace br_logger
