#include "../../include/br_logger/formatters/json_formatter.hpp"
#include "../../include/br_logger/log_level.hpp"
#include "../../include/br_logger/timestamp.hpp"
#include <cstdio>
#include <cstring>
#include <cinttypes>

static size_t safe_append(char* buf, size_t buf_size, size_t pos, const char* src, size_t src_len) {
    if (pos >= buf_size - 1) {
        return pos;
    }
    size_t avail = buf_size - 1 - pos;
    size_t n = (src_len < avail) ? src_len : avail;
    std::memcpy(buf + pos, src, n);
    return pos + n;
}

static size_t safe_append_str(char* buf, size_t buf_size, size_t pos, const char* s) {
    return safe_append(buf, buf_size, pos, s, std::strlen(s));
}

br_logger::JsonFormatter::JsonFormatter(bool pretty)
    : pretty_(pretty) {
}

size_t br_logger::JsonFormatter::EscapeJsonString(const char* src, size_t src_len, char* dst, size_t dst_size) {
    if (dst_size == 0) {
        return 0;
    }
    size_t pos = 0;
    const char* hex_digits = "0123456789ABCDEF";
    for (size_t i = 0; i < src_len; ++i) {
        unsigned char c = static_cast<unsigned char>(src[i]);
        if (pos >= dst_size) {
            break;
        }
        switch (c) {
            case '"':
                if (pos + 2 > dst_size) {
                    return pos;
                }
                dst[pos++] = '\\';
                dst[pos++] = '"';
                break;
            case '\\':
                if (pos + 2 > dst_size) {
                    return pos;
                }
                dst[pos++] = '\\';
                dst[pos++] = '\\';
                break;
            case '\n':
                if (pos + 2 > dst_size) {
                    return pos;
                }
                dst[pos++] = '\\';
                dst[pos++] = 'n';
                break;
            case '\r':
                if (pos + 2 > dst_size) {
                    return pos;
                }
                dst[pos++] = '\\';
                dst[pos++] = 'r';
                break;
            case '\t':
                if (pos + 2 > dst_size) {
                    return pos;
                }
                dst[pos++] = '\\';
                dst[pos++] = 't';
                break;
            default:
                if (c <= 0x1F) {
                    if (pos + 6 > dst_size) {
                        return pos;
                    }
                    dst[pos++] = '\\';
                    dst[pos++] = 'u';
                    dst[pos++] = hex_digits[(c >> 12) & 0x0F];
                    dst[pos++] = hex_digits[(c >> 8) & 0x0F];
                    dst[pos++] = hex_digits[(c >> 4) & 0x0F];
                    dst[pos++] = hex_digits[c & 0x0F];
                } else {
                    if (pos + 1 > dst_size) {
                        return pos;
                    }
                    dst[pos++] = static_cast<char>(c);
                }
                break;
        }
    }
    return pos;
}

size_t br_logger::JsonFormatter::format(const LogEntry& entry, char* buf, size_t buf_size) {
    if (buf_size == 0) {
        return 0;
    }

    size_t pos = 0;
    char tmp[128];

    auto append_escaped = [&](const char* src, size_t src_len) {
        if (pos >= buf_size - 1) {
            return;
        }
        size_t avail = buf_size - 1 - pos;
        size_t n = EscapeJsonString(src, src_len, buf + pos, avail);
        if (n > avail) {
            n = avail;
        }
        pos += n;
    };

    const char* nl = pretty_ ? "\n" : "";
    const char* ind = pretty_ ? "  " : "";
    const char* sep = pretty_ ? ": " : ":";
    const char* comma = pretty_ ? ",\n" : ",";

    pos = safe_append(buf, buf_size, pos, "{", 1);
    pos = safe_append_str(buf, buf_size, pos, nl);

    pos = safe_append_str(buf, buf_size, pos, ind);
    pos = safe_append(buf, buf_size, pos, "\"ts\"", 4);
    pos = safe_append_str(buf, buf_size, pos, sep);
    pos = safe_append(buf, buf_size, pos, "\"", 1);
    size_t ts_len = format_timestamp(entry.wall_clock_ns, tmp, sizeof(tmp));
    pos = safe_append(buf, buf_size, pos, tmp, ts_len);
    pos = safe_append(buf, buf_size, pos, "\"", 1);
    pos = safe_append_str(buf, buf_size, pos, comma);

    pos = safe_append_str(buf, buf_size, pos, ind);
    pos = safe_append(buf, buf_size, pos, "\"level\"", 7);
    pos = safe_append_str(buf, buf_size, pos, sep);
    pos = safe_append(buf, buf_size, pos, "\"", 1);
    auto level_sv = to_string(entry.level);
    pos = safe_append(buf, buf_size, pos, level_sv.data(), level_sv.size());
    pos = safe_append(buf, buf_size, pos, "\"", 1);
    pos = safe_append_str(buf, buf_size, pos, comma);

    pos = safe_append_str(buf, buf_size, pos, ind);
    pos = safe_append(buf, buf_size, pos, "\"file\"", 6);
    pos = safe_append_str(buf, buf_size, pos, sep);
    pos = safe_append(buf, buf_size, pos, "\"", 1);
    if (entry.file_name) {
        append_escaped(entry.file_name, std::strlen(entry.file_name));
    }
    pos = safe_append(buf, buf_size, pos, "\"", 1);
    pos = safe_append_str(buf, buf_size, pos, comma);

    pos = safe_append_str(buf, buf_size, pos, ind);
    pos = safe_append(buf, buf_size, pos, "\"line\"", 6);
    pos = safe_append_str(buf, buf_size, pos, sep);
    int line_n = snprintf(tmp, sizeof(tmp), "%u", entry.line);
    if (line_n > 0) {
        pos = safe_append(buf, buf_size, pos, tmp, static_cast<size_t>(line_n));
    }
    pos = safe_append_str(buf, buf_size, pos, comma);

    pos = safe_append_str(buf, buf_size, pos, ind);
    pos = safe_append(buf, buf_size, pos, "\"func\"", 6);
    pos = safe_append_str(buf, buf_size, pos, sep);
    pos = safe_append(buf, buf_size, pos, "\"", 1);
    if (entry.function_name) {
        append_escaped(entry.function_name, std::strlen(entry.function_name));
    }
    pos = safe_append(buf, buf_size, pos, "\"", 1);
    pos = safe_append_str(buf, buf_size, pos, comma);

    pos = safe_append_str(buf, buf_size, pos, ind);
    pos = safe_append(buf, buf_size, pos, "\"tid\"", 5);
    pos = safe_append_str(buf, buf_size, pos, sep);
    int tid_n = snprintf(tmp, sizeof(tmp), "%u", entry.thread_id);
    if (tid_n > 0) {
        pos = safe_append(buf, buf_size, pos, tmp, static_cast<size_t>(tid_n));
    }
    pos = safe_append_str(buf, buf_size, pos, comma);

    pos = safe_append_str(buf, buf_size, pos, ind);
    pos = safe_append(buf, buf_size, pos, "\"pid\"", 5);
    pos = safe_append_str(buf, buf_size, pos, sep);
    int pid_n = snprintf(tmp, sizeof(tmp), "%u", entry.process_id);
    if (pid_n > 0) {
        pos = safe_append(buf, buf_size, pos, tmp, static_cast<size_t>(pid_n));
    }
    pos = safe_append_str(buf, buf_size, pos, comma);

    pos = safe_append_str(buf, buf_size, pos, ind);
    pos = safe_append(buf, buf_size, pos, "\"thread\"", 8);
    pos = safe_append_str(buf, buf_size, pos, sep);
    pos = safe_append(buf, buf_size, pos, "\"", 1);
    append_escaped(entry.thread_name, std::strlen(entry.thread_name));
    pos = safe_append(buf, buf_size, pos, "\"", 1);
    pos = safe_append_str(buf, buf_size, pos, comma);

    pos = safe_append_str(buf, buf_size, pos, ind);
    pos = safe_append(buf, buf_size, pos, "\"seq\"", 5);
    pos = safe_append_str(buf, buf_size, pos, sep);
    int seq_n = snprintf(tmp, sizeof(tmp), "%" PRIu64, entry.sequence_id);
    if (seq_n > 0) {
        pos = safe_append(buf, buf_size, pos, tmp, static_cast<size_t>(seq_n));
    }
    pos = safe_append_str(buf, buf_size, pos, comma);

    pos = safe_append_str(buf, buf_size, pos, ind);
    pos = safe_append(buf, buf_size, pos, "\"tags\"", 6);
    pos = safe_append_str(buf, buf_size, pos, sep);
    pos = safe_append(buf, buf_size, pos, "{", 1);
    if (entry.tag_count > 0) {
        for (uint8_t t = 0; t < entry.tag_count; ++t) {
            if (t > 0) {
                pos = safe_append(buf, buf_size, pos, ",", 1);
            }
            pos = safe_append(buf, buf_size, pos, "\"", 1);
            append_escaped(entry.tags[t].key, std::strlen(entry.tags[t].key));
            pos = safe_append(buf, buf_size, pos, "\"", 1);
            pos = safe_append_str(buf, buf_size, pos, sep);
            pos = safe_append(buf, buf_size, pos, "\"", 1);
            append_escaped(entry.tags[t].value, std::strlen(entry.tags[t].value));
            pos = safe_append(buf, buf_size, pos, "\"", 1);
        }
    }
    pos = safe_append(buf, buf_size, pos, "}", 1);
    pos = safe_append_str(buf, buf_size, pos, comma);

    pos = safe_append_str(buf, buf_size, pos, ind);
    pos = safe_append(buf, buf_size, pos, "\"msg\"", 5);
    pos = safe_append_str(buf, buf_size, pos, sep);
    pos = safe_append(buf, buf_size, pos, "\"", 1);
    append_escaped(entry.msg, entry.msg_len);
    pos = safe_append(buf, buf_size, pos, "\"", 1);
    pos = safe_append_str(buf, buf_size, pos, nl);

    pos = safe_append(buf, buf_size, pos, "}", 1);

    buf[pos] = '\0';
    return pos;
}
