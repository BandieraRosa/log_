#include "../../include/br_logger/formatters/pattern_formatter.hpp"
#include "../../include/br_logger/log_level.hpp"
#include "../../include/br_logger/timestamp.hpp"
#include <cstdio>
#include <cstring>
#include <ctime>
#include <cinttypes>

br_logger::PatternFormatter::PatternFormatter(std::string_view pattern, bool enable_color)
    : pattern_(pattern), enable_color_(enable_color) {
    compile_pattern();
}

void br_logger::PatternFormatter::compile_pattern() {
    ops_.clear();
    std::string literal;

    auto flush_literal = [&]() {
        if (!literal.empty()) {
            ops_.push_back({OpType::Literal, literal});
            literal.clear();
        }
    };

    for (size_t i = 0; i < pattern_.size(); ++i) {
        char ch = pattern_[i];
        if (ch != '%') {
            literal.push_back(ch);
            continue;
        }

        if (i + 1 >= pattern_.size()) {
            literal.push_back('%');
            continue;
        }

        char next = pattern_[++i];
        switch (next) {
            case 'D': flush_literal(); ops_.push_back({OpType::Date, {}}); break;
            case 'T': flush_literal(); ops_.push_back({OpType::Time, {}}); break;
            case 'e': flush_literal(); ops_.push_back({OpType::Microseconds, {}}); break;
            case 'L': flush_literal(); ops_.push_back({OpType::LevelFull, {}}); break;
            case 'l': flush_literal(); ops_.push_back({OpType::LevelShort, {}}); break;
            case 'f': flush_literal(); ops_.push_back({OpType::FileName, {}}); break;
            case 'F': flush_literal(); ops_.push_back({OpType::FilePath, {}}); break;
            case 'n': flush_literal(); ops_.push_back({OpType::FuncName, {}}); break;
            case 'N': flush_literal(); ops_.push_back({OpType::PrettyFunc, {}}); break;
            case '#': flush_literal(); ops_.push_back({OpType::Line, {}}); break;
            case 't': flush_literal(); ops_.push_back({OpType::ThreadId, {}}); break;
            case 'P': flush_literal(); ops_.push_back({OpType::ProcessId, {}}); break;
            case 'k': flush_literal(); ops_.push_back({OpType::ThreadName, {}}); break;
            case 'q': flush_literal(); ops_.push_back({OpType::SequenceId, {}}); break;
            case 'g': flush_literal(); ops_.push_back({OpType::Tags, {}}); break;
            case 'm': flush_literal(); ops_.push_back({OpType::Message, {}}); break;
            case 'C': flush_literal(); ops_.push_back({OpType::ColorStart, {}}); break;
            case 'R': flush_literal(); ops_.push_back({OpType::ColorReset, {}}); break;
            case '%': literal.push_back('%'); break;
            default:
                literal.push_back('%');
                literal.push_back(next);
                break;
        }
    }

    flush_literal();
}

size_t br_logger::PatternFormatter::format(const LogEntry& entry, char* buf, size_t buf_size) {
    if (buf_size == 0) {
        return 0;
    }
    size_t pos = 0;
    buf[0] = '\0';

    auto append_data = [&](const char* data, size_t len) {
        if (buf_size == 0 || len == 0) {
            return;
        }
        size_t avail = (pos < buf_size) ? (buf_size - 1 - pos) : 0;
        if (avail == 0) {
            return;
        }
        size_t to_copy = (len < avail) ? len : avail;
        std::memcpy(buf + pos, data, to_copy);
        pos += to_copy;
        buf[pos] = '\0';
    };

    auto append_cstr = [&](const char* data) {
        if (!data) {
            return;
        }
        append_data(data, std::strlen(data));
    };

    for (const auto& op : ops_) {
        char temp[128];
        switch (op.type) {
            case OpType::Literal:
                append_data(op.literal.data(), op.literal.size());
                break;
            case OpType::Date: {
                size_t len = format_date(entry.wall_clock_ns, temp, sizeof(temp));
                append_data(temp, len);
                break;
            }
            case OpType::Time: {
                time_t sec = static_cast<time_t>(entry.wall_clock_ns / 1000000000ULL);
                struct tm tm_val{};
                (void)localtime_r(&sec, &tm_val);
                int n = std::snprintf(temp, sizeof(temp), "%02d:%02d:%02d",
                                      tm_val.tm_hour, tm_val.tm_min, tm_val.tm_sec);
                if (n > 0) {
                    append_data(temp, static_cast<size_t>(n));
                }
                break;
            }
            case OpType::Microseconds: {
                uint32_t us = static_cast<uint32_t>((entry.wall_clock_ns / 1000ULL) % 1000000ULL);
                int n = std::snprintf(temp, sizeof(temp), ".%06u", us);
                if (n > 0) {
                    append_data(temp, static_cast<size_t>(n));
                }
                break;
            }
            case OpType::LevelFull: {
                auto level = to_string(entry.level);
                append_data(level.data(), level.size());
                break;
            }
            case OpType::LevelShort: {
                char ch = to_short_char(entry.level);
                append_data(&ch, 1);
                break;
            }
            case OpType::FileName:
                append_cstr(entry.file_name);
                break;
            case OpType::FilePath:
                append_cstr(entry.file_path);
                break;
            case OpType::FuncName:
                append_cstr(entry.function_name);
                break;
            case OpType::PrettyFunc:
                append_cstr(entry.pretty_function);
                break;
            case OpType::Line: {
                int n = std::snprintf(temp, sizeof(temp), "%u", entry.line);
                if (n > 0) {
                    append_data(temp, static_cast<size_t>(n));
                }
                break;
            }
            case OpType::ThreadId: {
                int n = std::snprintf(temp, sizeof(temp), "%u", entry.thread_id);
                if (n > 0) {
                    append_data(temp, static_cast<size_t>(n));
                }
                break;
            }
            case OpType::ProcessId: {
                int n = std::snprintf(temp, sizeof(temp), "%u", entry.process_id);
                if (n > 0) {
                    append_data(temp, static_cast<size_t>(n));
                }
                break;
            }
            case OpType::ThreadName:
                append_cstr(entry.thread_name);
                break;
            case OpType::SequenceId: {
                int n = std::snprintf(temp, sizeof(temp), "%" PRIu64,
                                      static_cast<uint64_t>(entry.sequence_id));
                if (n > 0) {
                    append_data(temp, static_cast<size_t>(n));
                }
                break;
            }
            case OpType::Tags: {
                if (entry.tag_count > 0) {
                    append_data("[", 1);
                    for (uint8_t i = 0; i < entry.tag_count; ++i) {
                        if (i > 0) {
                            append_data("|", 1);
                        }
                        append_cstr(entry.tags[i].key);
                        append_data("=", 1);
                        append_cstr(entry.tags[i].value);
                    }
                    append_data("]", 1);
                }
                break;
            }
            case OpType::Message:
                append_data(entry.msg, entry.msg_len);
                break;
            case OpType::ColorStart: {
                if (!enable_color_) {
                    break;
                }
                const char* color = "";
                switch (entry.level) {
                    case br_logger::LogLevel::Trace: color = "\033[37m"; break;
                    case br_logger::LogLevel::Debug: color = "\033[36m"; break;
                    case br_logger::LogLevel::Info:  color = "\033[32m"; break;
                    case br_logger::LogLevel::Warn:  color = "\033[33m"; break;
                    case br_logger::LogLevel::Error: color = "\033[31m"; break;
                    case br_logger::LogLevel::Fatal: color = "\033[1;31m"; break;
                    case br_logger::LogLevel::Off:   color = ""; break;
                }
                append_cstr(color);
                break;
            }
            case OpType::ColorReset:
                if (enable_color_) {
                    append_cstr("\033[0m");
                }
                break;
        }
    }

    return pos;
}
