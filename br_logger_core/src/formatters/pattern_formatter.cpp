#include "br_logger/formatters/pattern_formatter.hpp"
#include "br_logger/log_level.hpp"
#include "br_logger/timestamp.hpp"
#include <cstdio>
#include <cstring>
#include <ctime>
#include <cinttypes>

namespace br_logger {

namespace {

size_t safe_append(char* buf, size_t buf_size, size_t pos, const char* src, size_t src_len) {
    if (pos >= buf_size - 1) return pos;
    size_t avail = buf_size - 1 - pos;
    size_t n = (src_len < avail) ? src_len : avail;
    std::memcpy(buf + pos, src, n);
    return pos + n;
}

size_t safe_append_str(char* buf, size_t buf_size, size_t pos, const char* s) {
    return safe_append(buf, buf_size, pos, s, std::strlen(s));
}

const char* color_for_level(LogLevel level) {
    switch (level) {
        case LogLevel::Trace: return "\033[37m";
        case LogLevel::Debug: return "\033[36m";
        case LogLevel::Info:  return "\033[32m";
        case LogLevel::Warn:  return "\033[33m";
        case LogLevel::Error: return "\033[31m";
        case LogLevel::Fatal: return "\033[1;31m";
        default:              return "";
    }
}

} // namespace

PatternFormatter::PatternFormatter(std::string_view pattern, bool enable_color)
    : pattern_(pattern), enable_color_(enable_color) {
    compile_pattern();
}

void PatternFormatter::compile_pattern() {
    ops_.clear();
    size_t i = 0;
    std::string literal_buf;

    auto flush_literal = [&]() {
        if (!literal_buf.empty()) {
            ops_.push_back({OpType::Literal, std::move(literal_buf)});
            literal_buf.clear();
        }
    };

    while (i < pattern_.size()) {
        if (pattern_[i] == '%' && i + 1 < pattern_.size()) {
            char c = pattern_[i + 1];
            OpType op = OpType::Literal;
            bool is_op = true;

            switch (c) {
                case 'D': op = OpType::Date; break;
                case 'T': op = OpType::Time; break;
                case 'e': op = OpType::Microseconds; break;
                case 'L': op = OpType::LevelFull; break;
                case 'l': op = OpType::LevelShort; break;
                case 'f': op = OpType::FileName; break;
                case 'F': op = OpType::FilePath; break;
                case 'n': op = OpType::FuncName; break;
                case 'N': op = OpType::PrettyFunc; break;
                case '#': op = OpType::Line; break;
                case 't': op = OpType::ThreadId; break;
                case 'P': op = OpType::ProcessId; break;
                case 'k': op = OpType::ThreadName; break;
                case 'q': op = OpType::SequenceId; break;
                case 'g': op = OpType::Tags; break;
                case 'm': op = OpType::Message; break;
                case 'C': op = OpType::ColorStart; break;
                case 'R': op = OpType::ColorReset; break;
                case '%': literal_buf += '%'; is_op = false; break;
                default:
                    literal_buf += '%';
                    literal_buf += c;
                    is_op = false;
                    break;
            }

            if (is_op) {
                flush_literal();
                ops_.push_back({op, {}});
            }
            i += 2;
        } else {
            literal_buf += pattern_[i];
            ++i;
        }
    }
    flush_literal();
}

size_t PatternFormatter::format(const LogEntry& entry, char* buf, size_t buf_size) {
    if (buf_size == 0) return 0;

    size_t pos = 0;
    char tmp[128];

    for (const auto& op : ops_) {
        switch (op.type) {
            case OpType::Literal:
                pos = safe_append(buf, buf_size, pos, op.literal.data(), op.literal.size());
                break;

            case OpType::Date: {
                size_t n = format_date(entry.wall_clock_ns, tmp, sizeof(tmp));
                pos = safe_append(buf, buf_size, pos, tmp, n);
                break;
            }

            case OpType::Time: {
                time_t sec = static_cast<time_t>(entry.wall_clock_ns / 1'000'000'000ULL);
                struct tm tm_val{};
                localtime_r(&sec, &tm_val);
                int n = snprintf(tmp, sizeof(tmp), "%02d:%02d:%02d",
                                 tm_val.tm_hour, tm_val.tm_min, tm_val.tm_sec);
                if (n > 0) pos = safe_append(buf, buf_size, pos, tmp, static_cast<size_t>(n));
                break;
            }

            case OpType::Microseconds: {
                uint32_t us = static_cast<uint32_t>((entry.wall_clock_ns / 1000ULL) % 1'000'000ULL);
                int n = snprintf(tmp, sizeof(tmp), ".%06u", us);
                if (n > 0) pos = safe_append(buf, buf_size, pos, tmp, static_cast<size_t>(n));
                break;
            }

            case OpType::LevelFull: {
                auto sv = to_string(entry.level);
                pos = safe_append(buf, buf_size, pos, sv.data(), sv.size());
                break;
            }

            case OpType::LevelShort: {
                char c = to_short_char(entry.level);
                pos = safe_append(buf, buf_size, pos, &c, 1);
                break;
            }

            case OpType::FileName:
                if (entry.file_name) pos = safe_append_str(buf, buf_size, pos, entry.file_name);
                break;

            case OpType::FilePath:
                if (entry.file_path) pos = safe_append_str(buf, buf_size, pos, entry.file_path);
                break;

            case OpType::FuncName:
                if (entry.function_name) pos = safe_append_str(buf, buf_size, pos, entry.function_name);
                break;

            case OpType::PrettyFunc:
                if (entry.pretty_function) pos = safe_append_str(buf, buf_size, pos, entry.pretty_function);
                break;

            case OpType::Line: {
                int n = snprintf(tmp, sizeof(tmp), "%u", entry.line);
                if (n > 0) pos = safe_append(buf, buf_size, pos, tmp, static_cast<size_t>(n));
                break;
            }

            case OpType::ThreadId: {
                int n = snprintf(tmp, sizeof(tmp), "%u", entry.thread_id);
                if (n > 0) pos = safe_append(buf, buf_size, pos, tmp, static_cast<size_t>(n));
                break;
            }

            case OpType::ProcessId: {
                int n = snprintf(tmp, sizeof(tmp), "%u", entry.process_id);
                if (n > 0) pos = safe_append(buf, buf_size, pos, tmp, static_cast<size_t>(n));
                break;
            }

            case OpType::ThreadName:
                pos = safe_append_str(buf, buf_size, pos, entry.thread_name);
                break;

            case OpType::SequenceId: {
                int n = snprintf(tmp, sizeof(tmp), "%" PRIu64, entry.sequence_id);
                if (n > 0) pos = safe_append(buf, buf_size, pos, tmp, static_cast<size_t>(n));
                break;
            }

            case OpType::Tags: {
                if (entry.tag_count > 0) {
                    pos = safe_append(buf, buf_size, pos, "[", 1);
                    for (uint8_t t = 0; t < entry.tag_count; ++t) {
                        if (t > 0) pos = safe_append(buf, buf_size, pos, "|", 1);
                        pos = safe_append_str(buf, buf_size, pos, entry.tags[t].key);
                        pos = safe_append(buf, buf_size, pos, "=", 1);
                        pos = safe_append_str(buf, buf_size, pos, entry.tags[t].value);
                    }
                    pos = safe_append(buf, buf_size, pos, "]", 1);
                }
                break;
            }

            case OpType::Message:
                pos = safe_append(buf, buf_size, pos, entry.msg, entry.msg_len);
                break;

            case OpType::ColorStart:
                if (enable_color_) {
                    pos = safe_append_str(buf, buf_size, pos, color_for_level(entry.level));
                }
                break;

            case OpType::ColorReset:
                if (enable_color_) {
                    pos = safe_append_str(buf, buf_size, pos, "\033[0m");
                }
                break;
        }
    }

    buf[pos] = '\0';
    return pos;
}

} // namespace br_logger
