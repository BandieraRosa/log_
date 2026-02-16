#include <gtest/gtest.h>
#include "br_logger/formatters/pattern_formatter.hpp"
#include "br_logger/log_level.hpp"
#include "br_logger/platform.hpp"
#include <cstring>
#include <string>

using br_logger::PatternFormatter;
using br_logger::LogEntry;
using br_logger::LogLevel;

namespace {

LogEntry make_test_entry() {
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

std::string format_with(const char* pattern, const LogEntry& entry, bool color = true) {
    PatternFormatter fmt(pattern, color);
    char buf[2048];
    size_t n = fmt.format(entry, buf, sizeof(buf));
    return std::string(buf, n);
}

}

TEST(PatternFormatter, LevelFull) {
    auto entry = make_test_entry();
    EXPECT_EQ(format_with("%L", entry), "INFO");

    entry.level = LogLevel::Warn;
    EXPECT_EQ(format_with("%L", entry), "WARN");
}

TEST(PatternFormatter, LevelShort) {
    auto entry = make_test_entry();
    EXPECT_EQ(format_with("%l", entry), "I");

    entry.level = LogLevel::Error;
    EXPECT_EQ(format_with("%l", entry), "E");
}

TEST(PatternFormatter, FileName) {
    auto entry = make_test_entry();
    EXPECT_EQ(format_with("%f", entry), "main.cpp");
}

TEST(PatternFormatter, FilePath) {
    auto entry = make_test_entry();
    EXPECT_EQ(format_with("%F", entry), "/src/main.cpp");
}

TEST(PatternFormatter, FuncName) {
    auto entry = make_test_entry();
    EXPECT_EQ(format_with("%n", entry), "process");
}

TEST(PatternFormatter, PrettyFunc) {
    auto entry = make_test_entry();
    EXPECT_EQ(format_with("%N", entry), "void process(int)");
}

TEST(PatternFormatter, Line) {
    auto entry = make_test_entry();
    EXPECT_EQ(format_with("%#", entry), "42");
}

TEST(PatternFormatter, ThreadId) {
    auto entry = make_test_entry();
    EXPECT_EQ(format_with("%t", entry), "1234");
}

TEST(PatternFormatter, ProcessId) {
    auto entry = make_test_entry();
    EXPECT_EQ(format_with("%P", entry), "5678");
}

TEST(PatternFormatter, ThreadName) {
    auto entry = make_test_entry();
    EXPECT_EQ(format_with("%k", entry), "worker");
}

TEST(PatternFormatter, SequenceId) {
    auto entry = make_test_entry();
    EXPECT_EQ(format_with("%q", entry), "1001");
}

TEST(PatternFormatter, Tags) {
    auto entry = make_test_entry();
    EXPECT_EQ(format_with("%g", entry), "[env=prod|req=abc123]");
}

TEST(PatternFormatter, EmptyTags) {
    auto entry = make_test_entry();
    entry.tag_count = 0;
    EXPECT_EQ(format_with("%g", entry), "");
}

TEST(PatternFormatter, Message) {
    auto entry = make_test_entry();
    EXPECT_EQ(format_with("%m", entry), "hello world");
}

TEST(PatternFormatter, EscapedPercent) {
    auto entry = make_test_entry();
    EXPECT_EQ(format_with("100%%", entry), "100%");
}

TEST(PatternFormatter, Literal) {
    auto entry = make_test_entry();
    EXPECT_EQ(format_with("hello", entry), "hello");
}

TEST(PatternFormatter, Date) {
    auto entry = make_test_entry();
    auto out = format_with("%D", entry);
    EXPECT_EQ(out.size(), 10u);
    EXPECT_EQ(out[4], '-');
    EXPECT_EQ(out[7], '-');
}

TEST(PatternFormatter, Time) {
    auto entry = make_test_entry();
    auto out = format_with("%T", entry);
    EXPECT_EQ(out.size(), 8u);
    EXPECT_EQ(out[2], ':');
    EXPECT_EQ(out[5], ':');
}

TEST(PatternFormatter, Microseconds) {
    auto entry = make_test_entry();
    auto out = format_with("%e", entry);
    EXPECT_EQ(out.size(), 7u);
    EXPECT_EQ(out[0], '.');
}

TEST(PatternFormatter, ColorInfo) {
    auto entry = make_test_entry();
    auto out = format_with("%C%L%R", entry, true);
    EXPECT_NE(out.find("\033[32m"), std::string::npos);
    EXPECT_NE(out.find("\033[0m"), std::string::npos);
    EXPECT_NE(out.find("INFO"), std::string::npos);
}

TEST(PatternFormatter, ColorWarn) {
    auto entry = make_test_entry();
    entry.level = LogLevel::Warn;
    auto out = format_with("%C%L%R", entry, true);
    EXPECT_NE(out.find("\033[33m"), std::string::npos);
}

TEST(PatternFormatter, ColorError) {
    auto entry = make_test_entry();
    entry.level = LogLevel::Error;
    auto out = format_with("%C%L%R", entry, true);
    EXPECT_NE(out.find("\033[31m"), std::string::npos);
}

TEST(PatternFormatter, ColorFatal) {
    auto entry = make_test_entry();
    entry.level = LogLevel::Fatal;
    auto out = format_with("%C%L%R", entry, true);
    EXPECT_NE(out.find("\033[1;31m"), std::string::npos);
}

TEST(PatternFormatter, ColorDisabled) {
    auto entry = make_test_entry();
    auto out = format_with("%C%L%R", entry, false);
    EXPECT_EQ(out.find("\033["), std::string::npos);
    EXPECT_EQ(out, "INFO");
}

TEST(PatternFormatter, DefaultPattern) {
    auto entry = make_test_entry();
    PatternFormatter fmt;
    char buf[2048];
    size_t n = fmt.format(entry, buf, sizeof(buf));
    std::string out(buf, n);
    EXPECT_FALSE(out.empty());
    EXPECT_NE(out.find("INFO"), std::string::npos);
    EXPECT_NE(out.find("main.cpp"), std::string::npos);
    EXPECT_NE(out.find("hello world"), std::string::npos);
}

TEST(PatternFormatter, BufferOverflow) {
    auto entry = make_test_entry();
    PatternFormatter fmt("%L %m");
    char buf[8];
    size_t n = fmt.format(entry, buf, sizeof(buf));
    EXPECT_LT(n, sizeof(buf));
    EXPECT_EQ(buf[n], '\0');
}

TEST(PatternFormatter, CombinedPattern) {
    auto entry = make_test_entry();
    auto out = format_with("[%L] %f:%# %m", entry, false);
    EXPECT_EQ(out, "[INFO] main.cpp:42 hello world");
}
