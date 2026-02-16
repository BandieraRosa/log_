#include "br_logger/logger.hpp"

namespace br_logger
{

Logger& Logger::Instance()
{
  static Logger inst;
  return inst;
}

Logger::Logger() = default;

Logger::~Logger() { Stop(); }

void Logger::AddSink(std::unique_ptr<ILogSink> sink)
{
  backend_.AddSink(std::move(sink));
}

void Logger::SetLevel(LogLevel level) { level_.store(level, std::memory_order_relaxed); }

LogLevel Logger::Level() const { return level_.load(std::memory_order_relaxed); }

void Logger::Start()
{
  if (started_)
  {
    return;
  }
  backend_.Start();
  started_ = true;
}

void Logger::Stop()
{
  if (!started_)
  {
    return;
  }
  backend_.Stop();
  started_ = false;
}

size_t Logger::Drain(size_t max_entries) { return backend_.Drain(max_entries); }

uint64_t Logger::DropCount() const { return drop_count_.load(std::memory_order_relaxed); }

void Logger::ResetDropCount() { drop_count_.store(0, std::memory_order_relaxed); }

}  // namespace br_logger
