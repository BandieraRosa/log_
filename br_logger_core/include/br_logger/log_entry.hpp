#pragma once
#include "log_level.hpp"
#include "platform.hpp"
#include <cstdint>
#include <type_traits>

namespace br_logger {

struct LogTag {
    char key[BR_LOG_MAX_TAG_KEY_LEN];
    char value[BR_LOG_MAX_TAG_VAL_LEN];
};

struct LogEntry {
    uint64_t    timestamp_ns;
    uint64_t    wall_clock_ns;

    LogLevel    level;

    const char* file_path;
    const char* file_name;
    const char* function_name;
    const char* pretty_function;
    uint32_t    line;
    uint32_t    column;

    uint32_t    thread_id;
    uint32_t    process_id;
    char        thread_name[32];

    uint8_t     tag_count;
    LogTag      tags[BR_LOG_MAX_TAGS];

    uint64_t    sequence_id;

    uint16_t    msg_len;
    char        msg[BR_LOG_MAX_MSG_LEN];
};

static_assert(std::is_trivially_copyable_v<LogEntry>,
    "LogEntry must be trivially copyable for lock-free ring buffer");

} // namespace br_logger
