#pragma once
#include "ring_buffer.hpp"
#include "log_entry.hpp"
#include "sinks/sink_interface.hpp"
#include "platform.hpp"
#include <vector>
#include <memory>
#include <atomic>

#if BR_LOG_HAS_THREAD
#include <thread>
#endif

namespace br_logger {

class LoggerBackend {
public:
    LoggerBackend();
    ~LoggerBackend();

    // 生产者调用（业务线程）
    bool try_push(const LogEntry& entry);

    // 管理 Sink
    void add_sink(std::unique_ptr<ILogSink> sink);

    // 启动/停止后端线程
    void start();
    void stop();    // 设置 running_ = false，等待线程退出，drain 残留日志

    // 嵌入式模式：无线程，手动调用 drain
    size_t drain(size_t max_entries = 64);

private:
    MPSCRingBuffer<LogEntry, BR_LOG_RING_SIZE> ring_;
    std::vector<std::unique_ptr<ILogSink>> sinks_;
    std::atomic<bool> running_{false};

#if BR_LOG_HAS_THREAD
    std::thread worker_;
    void worker_loop();
#endif

    // 将一条 entry 分发到所有 sink
    void dispatch(const LogEntry& entry);
};

} // namespace br_logger
