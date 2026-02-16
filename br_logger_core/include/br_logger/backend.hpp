#pragma once
#include <atomic>
#include <memory>
#include <vector>

#include "log_entry.hpp"
#include "platform.hpp"
#include "ring_buffer.hpp"
#include "sinks/sink_interface.hpp"

#if BR_LOG_HAS_THREAD
#include <thread>
#endif

namespace br_logger
{

class LoggerBackend
{
 public:
  LoggerBackend();
  ~LoggerBackend();

  // 生产者调用（业务线程）
  bool TryPush(const LogEntry& entry);

  // 管理 Sink
  void AddSink(std::unique_ptr<ILogSink> sink);

  // 启动/停止后端线程
  void Start();
  void Stop();  // 设置 running_ = false，等待线程退出，drain 残留日志

  // 嵌入式模式：无线程，手动调用 drain
  size_t Drain(size_t max_entries = 64);

 private:
  MPSCRingBuffer<LogEntry, BR_LOG_RING_SIZE> ring_;
  std::vector<std::unique_ptr<ILogSink>> sinks_;
  std::atomic<bool> running_{false};

#if BR_LOG_HAS_THREAD
  std::thread worker_;
  void WorkerLoop();
#endif

  // 将一条 entry 分发到所有 sink
  void Dispatch(const LogEntry& entry);
};

}  // namespace br_logger
