#include "br_logger/backend.hpp"

#if BR_LOG_HAS_THREAD
#include <chrono>
#endif

namespace br_logger
{

LoggerBackend::LoggerBackend() = default;

LoggerBackend::~LoggerBackend() { Stop(); }

bool LoggerBackend::TryPush(const LogEntry& entry) { return ring_.TryPush(entry); }

void LoggerBackend::AddSink(std::unique_ptr<ILogSink> sink)
{
  sinks_.push_back(std::move(sink));
}

void LoggerBackend::Start()
{
  if (running_.load(std::memory_order_relaxed))
  {
    return;
  }
  running_.store(true, std::memory_order_relaxed);
#if BR_LOG_HAS_THREAD
  worker_ = std::thread(&LoggerBackend::WorkerLoop, this);
#endif
}

void LoggerBackend::Stop()
{
  running_.store(false, std::memory_order_relaxed);
#if BR_LOG_HAS_THREAD
  if (worker_.joinable())
  {
    worker_.join();
  }
#endif
  while (Drain(64) > 0)
  {
  }
  for (auto& sink : sinks_)
  {
    sink->Flush();
  }
}

size_t LoggerBackend::Drain(size_t max_entries)
{
  size_t count = 0;
  LogEntry entry{};
  while (count < max_entries && ring_.TryPop(entry))
  {
    Dispatch(entry);
    ++count;
  }
  return count;
}

void LoggerBackend::Dispatch(const LogEntry& entry)
{
  for (auto& sink : sinks_)
  {
    sink->Write(entry);
  }
}

#if BR_LOG_HAS_THREAD
void LoggerBackend::WorkerLoop()
{
  uint32_t idle_count = 0;
  while (running_.load(std::memory_order_relaxed))
  {
    size_t drained = Drain(64);
    if (drained > 0)
    {
      idle_count = 0;
    }
    else
    {
      ++idle_count;
      if (idle_count < 100)
      {
        // busy spin
      }
      else if (idle_count < 1000)
      {
        std::this_thread::yield();
      }
      else
      {
        std::this_thread::sleep_for(std::chrono::microseconds(100));
      }
    }
  }
  while (Drain(64) > 0)
  {
  }
}
#endif

}  // namespace br_logger
