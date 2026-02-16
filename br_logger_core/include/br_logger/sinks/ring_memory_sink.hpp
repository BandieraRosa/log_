#pragma once
#include <cstddef>
#include <vector>

#include "sink_interface.hpp"

namespace br_logger
{

class RingMemorySink : public ILogSink
{
 public:
  explicit RingMemorySink(size_t capacity = 1024);

  void Write(const LogEntry& entry) override;
  void Flush() override;

  bool DumpToFile(const char* path);

  size_t Size() const;

  const LogEntry& At(size_t index) const;

 private:
  std::vector<LogEntry> buffer_;
  size_t capacity_;
  size_t head_;
  size_t count_;
};

}  // namespace br_logger
