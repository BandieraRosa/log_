#pragma once
#include "../log_entry.hpp"
#include "../formatters/formatter_interface.hpp"
#include "../log_level.hpp"
#include <memory>

namespace br_logger {

class ILogSink {
public:
    virtual ~ILogSink() = default;

    // 写入一条日志（由后端线程调用）
    virtual void write(const LogEntry& entry) = 0;

    // 刷新缓冲区
    virtual void flush() = 0;

    // 设置该 Sink 的格式化器
    void set_formatter(std::unique_ptr<IFormatter> formatter) {
        formatter_ = std::move(formatter);
    }

    // 设置该 Sink 的最低输出级别（独立于全局级别）
    void set_level(LogLevel level) {
        min_level_ = level;
    }

    LogLevel level() const {
        return min_level_;
    }

    // Sink 级别过滤
    bool should_log(LogLevel entry_level) const {
        return entry_level >= min_level_;
    }

protected:
    std::unique_ptr<IFormatter> formatter_;
    LogLevel min_level_ = LogLevel::Trace;
    char format_buf_[2048];

    // 通用格式化，返回格式化后的长度
    size_t do_format(const LogEntry& entry) {
        if (formatter_) {
            return formatter_->format(entry, format_buf_, sizeof(format_buf_));
        }
        return 0;
    }
};

} // namespace br_logger
