#include "br_logger/logger.hpp"

namespace br_logger {

Logger& Logger::instance() {
    static Logger inst;
    return inst;
}

Logger::Logger() = default;

Logger::~Logger() {
    stop();
}

void Logger::add_sink(std::unique_ptr<ILogSink> sink) {
    backend_.add_sink(std::move(sink));
}

void Logger::set_level(LogLevel level) {
    level_.store(level, std::memory_order_relaxed);
}

LogLevel Logger::level() const {
    return level_.load(std::memory_order_relaxed);
}

void Logger::start() {
    if (started_) return;
    backend_.start();
    started_ = true;
}

void Logger::stop() {
    if (!started_) return;
    backend_.stop();
    started_ = false;
}

size_t Logger::drain(size_t max_entries) {
    return backend_.drain(max_entries);
}

uint64_t Logger::drop_count() const {
    return drop_count_.load(std::memory_order_relaxed);
}

void Logger::reset_drop_count() {
    drop_count_.store(0, std::memory_order_relaxed);
}

} // namespace br_logger
