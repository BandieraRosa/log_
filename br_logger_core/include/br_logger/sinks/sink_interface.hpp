#pragma once
#include <memory>

#include "../formatters/formatter_interface.hpp"
#include "../log_entry.hpp"
#include "../log_level.hpp"

namespace br_logger
{

class ILogSink
{
 public:
  virtual ~ILogSink() = default;

  // 写入一条日志（由后端线程调用）
  virtual void Write(const LogEntry& entry) = 0;

  // 刷新缓冲区
  virtual void Flush() = 0;

  // 设置该 Sink 的格式化器
  void SetFormatter(std::unique_ptr<IFormatter> formatter)
  {
    formatter_ = std::move(formatter);
  }

  // 设置该 Sink 的最低输出级别（独立于全局级别）
  void SetLevel(LogLevel level) { min_level_ = level; }

  LogLevel Level() const { return min_level_; }

  // Sink 级别过滤
  bool ShouldLog(LogLevel entry_level) const { return entry_level >= min_level_; }

 protected:
  std::unique_ptr<IFormatter> formatter_;
  LogLevel min_level_ = LogLevel::Trace;
  char format_buf_[2048];

  // 通用格式化，返回格式化后的长度
  size_t DoFormat(const LogEntry& entry)
  {
    if (formatter_)
    {
      return formatter_->Format(entry, format_buf_, sizeof(format_buf_));
    }
    return 0;
  }
};

}  // namespace br_logger
