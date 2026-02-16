#pragma once
#include <cstddef>

#include "../log_entry.hpp"

namespace br_logger
{

class IFormatter
{
 public:
  virtual ~IFormatter() = default;
  virtual size_t Format(const LogEntry& entry, char* buf, size_t buf_size) = 0;
};

}  // namespace br_logger
