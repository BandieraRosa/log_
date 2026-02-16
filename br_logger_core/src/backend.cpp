#include "br_logger/backend.hpp"

#if BR_LOG_HAS_THREAD
#include <chrono>
#endif

namespace br_logger {

LoggerBackend::LoggerBackend() = default;

LoggerBackend::~LoggerBackend() {
    stop();
}

bool LoggerBackend::try_push(const LogEntry& entry) {
    return ring_.try_push(entry);
}

void LoggerBackend::add_sink(std::unique_ptr<ILogSink> sink) {
    sinks_.push_back(std::move(sink));
}

void LoggerBackend::start() {
    if (running_.load(std::memory_order_relaxed)) {
        return;
    }
    running_.store(true, std::memory_order_relaxed);
#if BR_LOG_HAS_THREAD
    worker_ = std::thread(&LoggerBackend::worker_loop, this);
#endif
}

void LoggerBackend::stop() {
    running_.store(false, std::memory_order_relaxed);
#if BR_LOG_HAS_THREAD
    if (worker_.joinable()) {
        worker_.join();
    }
#endif
    while (drain(64) > 0) {}
    for (auto& sink : sinks_) {
        sink->flush();
    }
}

size_t LoggerBackend::drain(size_t max_entries) {
    size_t count = 0;
    LogEntry entry{};
    while (count < max_entries && ring_.try_pop(entry)) {
        dispatch(entry);
        ++count;
    }
    return count;
}

void LoggerBackend::dispatch(const LogEntry& entry) {
    for (auto& sink : sinks_) {
        sink->write(entry);
    }
}

#if BR_LOG_HAS_THREAD
void LoggerBackend::worker_loop() {
    uint32_t idle_count = 0;
    while (running_.load(std::memory_order_relaxed)) {
        size_t drained = drain(64);
        if (drained > 0) {
            idle_count = 0;
        } else {
            ++idle_count;
            if (idle_count < 100) {
                // busy spin
            } else if (idle_count < 1000) {
                std::this_thread::yield();
            } else {
                std::this_thread::sleep_for(std::chrono::microseconds(100));
            }
        }
    }
    while (drain(64) > 0) {}
}
#endif

} // namespace br_logger
