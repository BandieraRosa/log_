#pragma once
#include "log_level.hpp"
#include "log_entry.hpp"
#include "log_context.hpp"
#include "source_location.hpp"
#include "backend.hpp"
#include "timestamp.hpp"
#include "sinks/sink_interface.hpp"

#include <atomic>
#include <memory>
#include <cstdio>  // snprintf fallback

#ifdef BR_LOG_USE_FMTLIB
#include <fmt/format.h>
#endif

namespace br_logger {

class Logger {
public:
    static Logger& instance();

    void add_sink(std::unique_ptr<ILogSink> sink);
    void set_level(LogLevel level);
    LogLevel level() const;

    void start();
    void stop();

    size_t drain(size_t max_entries = 64);

    uint64_t drop_count() const;
    void reset_drop_count();

    // Core log method — template, defined in header
    template <typename... Args>
    void log_impl(LogLevel level, const SourceLocation& loc,
                  const char* fmt, Args&&... args);

private:
    Logger();
    ~Logger();

    LoggerBackend backend_;
    std::atomic<LogLevel> level_{LogLevel::Info};
    std::atomic<uint64_t> sequence_{0};
    std::atomic<uint64_t> drop_count_{0};
    bool started_ = false;
};

// ===== log_impl template implementation =====

template <typename... Args>
void Logger::log_impl(LogLevel level, const SourceLocation& loc,
                      const char* fmt, Args&&... args) {
    LogEntry entry{};

    // 1. Timestamps
    entry.timestamp_ns  = monotonic_now_ns();
    entry.wall_clock_ns = wall_clock_now_ns();

    // 2. Level
    entry.level = level;

    // 3. Source location
    entry.file_path      = loc.file_path;
    entry.file_name      = loc.file_name;
    entry.function_name  = loc.function_name;
    entry.pretty_function = loc.pretty_function;
    entry.line           = loc.line;
    entry.column         = loc.column;

    // 4. Sequence
    entry.sequence_id = sequence_.fetch_add(1, std::memory_order_relaxed);

    // 5. Context (thread info + tags)
    auto& ctx = LogContext::instance();
    ctx.fill_thread_info(entry);
    ctx.fill_tags(entry);

    // 6. Format message
#ifdef BR_LOG_USE_FMTLIB
    auto result = fmt::format_to_n(entry.msg, BR_LOG_MAX_MSG_LEN - 1,
                                   fmt::runtime(fmt), std::forward<Args>(args)...);
    entry.msg_len = static_cast<uint16_t>(result.size);
    entry.msg[entry.msg_len] = '\0';
#else
    int written = std::snprintf(entry.msg, BR_LOG_MAX_MSG_LEN, fmt, args...);
    entry.msg_len = (written > 0) ? static_cast<uint16_t>(written) : 0;
#endif

    // 7. Enqueue
    if (!backend_.try_push(entry)) {
        drop_count_.fetch_add(1, std::memory_order_relaxed);
    }
}

} // namespace br_logger

// ===== Logging macros =====

#define BR_LOG_CALL(lvl, fmt_str, ...) \
    do { \
        constexpr auto _hpc_lvl = ::br_logger::LogLevel::lvl; \
        if (static_cast<int>(_hpc_lvl) >= BR_LOG_ACTIVE_LEVEL) { \
            auto& _br_logger = ::br_logger::Logger::instance(); \
            if (_hpc_lvl >= _br_logger.level()) { \
                _br_logger.log_impl( \
                    _hpc_lvl, BR_LOG_CURRENT_LOCATION(), \
                    fmt_str, ##__VA_ARGS__); \
            } \
        } \
    } while (0)

#define LOG_TRACE(fmt, ...) BR_LOG_CALL(Trace, fmt, ##__VA_ARGS__)
#define LOG_DEBUG(fmt, ...) BR_LOG_CALL(Debug, fmt, ##__VA_ARGS__)
#define LOG_INFO(fmt, ...)  BR_LOG_CALL(Info,  fmt, ##__VA_ARGS__)
#define LOG_WARN(fmt, ...)  BR_LOG_CALL(Warn,  fmt, ##__VA_ARGS__)
#define LOG_ERROR(fmt, ...) BR_LOG_CALL(Error, fmt, ##__VA_ARGS__)
#define LOG_FATAL(fmt, ...) BR_LOG_CALL(Fatal, fmt, ##__VA_ARGS__)

// Conditional logging
#define LOG_INFO_IF(cond, fmt, ...)  do { if (cond) LOG_INFO(fmt, ##__VA_ARGS__); } while(0)
#define LOG_WARN_IF(cond, fmt, ...)  do { if (cond) LOG_WARN(fmt, ##__VA_ARGS__); } while(0)
#define LOG_ERROR_IF(cond, fmt, ...) do { if (cond) LOG_ERROR(fmt, ##__VA_ARGS__); } while(0)

// Rate-limited logging
#define LOG_EVERY_N(lvl, n, fmt, ...) \
    do { \
        static std::atomic<uint64_t> _hpc_count{0}; \
        if (_hpc_count.fetch_add(1, std::memory_order_relaxed) % (n) == 0) { \
            BR_LOG_CALL(lvl, fmt, ##__VA_ARGS__); \
        } \
    } while(0)

#define LOG_ONCE(lvl, fmt, ...) \
    do { \
        static std::atomic<bool> _br_logged{false}; \
        if (!_br_logged.exchange(true, std::memory_order_relaxed)) { \
            BR_LOG_CALL(lvl, fmt, ##__VA_ARGS__); \
        } \
    } while(0)
