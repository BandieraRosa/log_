#include <gtest/gtest.h>
#include "../include/br_logger/formatters/json_formatter.hpp"
#include "../include/br_logger/log_level.hpp"
#include "../include/br_logger/platform.hpp"
#include <array>
#include <cstring>
#include <string>

using br_logger::JsonFormatter;
using br_logger::LogEntry;
using br_logger::LogLevel;

static LogEntry make_test_entry() {
    LogEntry entry{};
    entry.wall_clock_ns = 1739692200123456000ULL;
    entry.timestamp_ns = 123456789ULL;
    entry.level = LogLevel::Info;
    entry.file_path = "/src/main.cpp";
    entry.file_name = "main.cpp";
    entry.function_name = "process";
    entry.pretty_function = "void process(int)";
    entry.line = 42;
    entry.column = 0;
    entry.thread_id = 1234;
    entry.process_id = 5678;
    std::strncpy(entry.thread_name, "worker", sizeof(entry.thread_name));
    entry.tag_count = 2;
    std::strncpy(entry.tags[0].key, "env", BR_LOG_MAX_TAG_KEY_LEN);
    std::strncpy(entry.tags[0].value, "prod", BR_LOG_MAX_TAG_VAL_LEN);
    std::strncpy(entry.tags[1].key, "req", BR_LOG_MAX_TAG_KEY_LEN);
    std::strncpy(entry.tags[1].value, "abc123", BR_LOG_MAX_TAG_VAL_LEN);
    entry.sequence_id = 1001;
    const char* msg = "hello world";
    entry.msg_len = static_cast<uint16_t>(std::strlen(msg));
    std::strncpy(entry.msg, msg, BR_LOG_MAX_MSG_LEN);
    return entry;
}

static std::string format_with(const LogEntry& entry, bool pretty = false, size_t buf_size = 2048) {
    JsonFormatter fmt(pretty);
    std::string out;
    out.resize(buf_size);
    size_t n = fmt.format(entry, out.data(), out.size());
    out.resize(n);
    return out;
}

TEST(JsonFormatter, BasicOutput) {
    auto entry = make_test_entry();
    auto out = format_with(entry);
    ASSERT_FALSE(out.empty());
    EXPECT_EQ(out.front(), '{');
    EXPECT_EQ(out.back(), '}');
}

TEST(JsonFormatter, ContainsAllFields) {
    auto entry = make_test_entry();
    auto out = format_with(entry);
    EXPECT_NE(out.find("\"ts\""), std::string::npos);
    EXPECT_NE(out.find("\"level\""), std::string::npos);
    EXPECT_NE(out.find("\"file\""), std::string::npos);
    EXPECT_NE(out.find("\"line\""), std::string::npos);
    EXPECT_NE(out.find("\"func\""), std::string::npos);
    EXPECT_NE(out.find("\"tid\""), std::string::npos);
    EXPECT_NE(out.find("\"pid\""), std::string::npos);
    EXPECT_NE(out.find("\"thread\""), std::string::npos);
    EXPECT_NE(out.find("\"seq\""), std::string::npos);
    EXPECT_NE(out.find("\"tags\""), std::string::npos);
    EXPECT_NE(out.find("\"msg\""), std::string::npos);
}

TEST(JsonFormatter, CorrectValues) {
    auto entry = make_test_entry();
    auto out = format_with(entry);
    EXPECT_NE(out.find("\"level\":\"INFO\""), std::string::npos);
    EXPECT_NE(out.find("\"file\":\"main.cpp\""), std::string::npos);
    EXPECT_NE(out.find("\"line\":42"), std::string::npos);
    EXPECT_NE(out.find("\"func\":\"process\""), std::string::npos);
    EXPECT_NE(out.find("\"tid\":1234"), std::string::npos);
    EXPECT_NE(out.find("\"pid\":5678"), std::string::npos);
    EXPECT_NE(out.find("\"thread\":\"worker\""), std::string::npos);
    EXPECT_NE(out.find("\"seq\":1001"), std::string::npos);
    EXPECT_NE(out.find("\"msg\":\"hello world\""), std::string::npos);
}

TEST(JsonFormatter, JsonEscapeQuote) {
    auto entry = make_test_entry();
    const char* msg = "he\"llo";
    entry.msg_len = static_cast<uint16_t>(std::strlen(msg));
    std::strncpy(entry.msg, msg, BR_LOG_MAX_MSG_LEN);
    auto out = format_with(entry);
    EXPECT_NE(out.find("he\\\"llo"), std::string::npos);
}

TEST(JsonFormatter, JsonEscapeBackslash) {
    auto entry = make_test_entry();
    const char* msg = "path\\file";
    entry.msg_len = static_cast<uint16_t>(std::strlen(msg));
    std::strncpy(entry.msg, msg, BR_LOG_MAX_MSG_LEN);
    auto out = format_with(entry);
    EXPECT_NE(out.find("path\\\\file"), std::string::npos);
}

TEST(JsonFormatter, JsonEscapeNewline) {
    auto entry = make_test_entry();
    const char* msg = "line1\nline2";
    entry.msg_len = static_cast<uint16_t>(std::strlen(msg));
    std::strncpy(entry.msg, msg, BR_LOG_MAX_MSG_LEN);
    auto out = format_with(entry);
    EXPECT_NE(out.find("line1\\nline2"), std::string::npos);
}

TEST(JsonFormatter, JsonEscapeTab) {
    auto entry = make_test_entry();
    const char* msg = "col1\tcol2";
    entry.msg_len = static_cast<uint16_t>(std::strlen(msg));
    std::strncpy(entry.msg, msg, BR_LOG_MAX_MSG_LEN);
    auto out = format_with(entry);
    EXPECT_NE(out.find("col1\\tcol2"), std::string::npos);
}

TEST(JsonFormatter, EmptyTags) {
    auto entry = make_test_entry();
    entry.tag_count = 0;
    auto out = format_with(entry);
    EXPECT_NE(out.find("\"tags\":{}"), std::string::npos);
}

TEST(JsonFormatter, WithTags) {
    auto entry = make_test_entry();
    auto out = format_with(entry);
    EXPECT_NE(out.find("\"tags\":{\"env\":\"prod\",\"req\":\"abc123\"}"), std::string::npos);
}

TEST(JsonFormatter, PrettyMode) {
    auto entry = make_test_entry();
    auto out = format_with(entry, true);
    EXPECT_NE(out.find("{\n"), std::string::npos);
    EXPECT_NE(out.find("\n  \"level\""), std::string::npos);
    EXPECT_NE(out.find("\n}"), std::string::npos);
}

TEST(JsonFormatter, EmptyMessage) {
    auto entry = make_test_entry();
    entry.msg_len = 0;
    entry.msg[0] = '\0';
    auto out = format_with(entry);
    EXPECT_NE(out.find("\"msg\":\"\""), std::string::npos);
}

TEST(JsonFormatter, BufferOverflow) {
    auto entry = make_test_entry();
    auto out = format_with(entry, false, 16);
    EXPECT_LE(out.size(), 15u);
}

TEST(JsonFormatter, AllLevels) {
    auto entry = make_test_entry();
    auto level_pairs = std::array<std::pair<LogLevel, const char*>, 7>{
        std::pair<LogLevel, const char*>{LogLevel::Trace, "TRACE"},
        std::pair<LogLevel, const char*>{LogLevel::Debug, "DEBUG"},
        std::pair<LogLevel, const char*>{LogLevel::Info, "INFO"},
        std::pair<LogLevel, const char*>{LogLevel::Warn, "WARN"},
        std::pair<LogLevel, const char*>{LogLevel::Error, "ERROR"},
        std::pair<LogLevel, const char*>{LogLevel::Fatal, "FATAL"},
        std::pair<LogLevel, const char*>{LogLevel::Off, "OFF"}
    };
    for (const auto& item : level_pairs) {
        entry.level = item.first;
        auto out = format_with(entry);
        std::string needle = std::string("\"level\":\"") + item.second + "\"";
        EXPECT_NE(out.find(needle), std::string::npos);
    }
}
