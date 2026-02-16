#include <gtest/gtest.h>

#include <array>
#include <cstring>
#include <thread>

#include "br_logger/log_context.hpp"
#include "br_logger/log_entry.hpp"

using br_logger::LogContext;
using br_logger::LogEntry;

namespace {

bool has_tag(const LogEntry& entry, const char* key, const char* value) {
    for (uint8_t i = 0; i < entry.tag_count; ++i) {
        if (std::strcmp(entry.tags[i].key, key) == 0
            && std::strcmp(entry.tags[i].value, value) == 0) {
            return true;
        }
    }
    return false;
}

bool has_key(const LogEntry& entry, const char* key) {
    for (uint8_t i = 0; i < entry.tag_count; ++i) {
        if (std::strcmp(entry.tags[i].key, key) == 0) {
            return true;
        }
    }
    return false;
}

}

TEST(LogContext, GlobalTagSetAndFill) {
    LogContext& ctx = LogContext::instance();
    ctx.set_global_tag("env", "prod");

    LogEntry entry{};
    ctx.fill_tags(entry);

    EXPECT_TRUE(has_tag(entry, "env", "prod"));

    ctx.remove_global_tag("env");
}

TEST(LogContext, GlobalTagRemove) {
    LogContext& ctx = LogContext::instance();
    ctx.set_global_tag("stage", "blue");
    ctx.remove_global_tag("stage");

    LogEntry entry{};
    ctx.fill_tags(entry);

    EXPECT_FALSE(has_key(entry, "stage"));
}

TEST(LogContext, GlobalTagUpdate) {
    LogContext& ctx = LogContext::instance();
    ctx.set_global_tag("region", "us");
    ctx.set_global_tag("region", "eu");

    LogEntry entry{};
    ctx.fill_tags(entry);

    EXPECT_EQ(entry.tag_count, 1);
    EXPECT_TRUE(has_tag(entry, "region", "eu"));

    ctx.remove_global_tag("region");
}

TEST(LogContext, TLSTagPushAndFill) {
    LogContext::push_scoped_tag("req", "123");

    LogEntry entry{};
    LogContext::instance().fill_tags(entry);
    EXPECT_TRUE(has_tag(entry, "req", "123"));

    LogContext::pop_scoped_tag("req");
    LogEntry entry_after{};
    LogContext::instance().fill_tags(entry_after);
    EXPECT_FALSE(has_key(entry_after, "req"));
}

TEST(LogContext, ScopedTagRAII) {
    LogEntry entry_inside{};
    {
        LogContext::ScopedTag scoped("trace", "abc");
        LogContext::instance().fill_tags(entry_inside);
        EXPECT_TRUE(has_tag(entry_inside, "trace", "abc"));
    }

    LogEntry entry_outside{};
    LogContext::instance().fill_tags(entry_outside);
    EXPECT_FALSE(has_key(entry_outside, "trace"));
}

TEST(LogContext, FillTagsCombinesGlobalAndTLS) {
    LogContext& ctx = LogContext::instance();
    ctx.set_global_tag("env", "dev");
    LogContext::push_scoped_tag("req", "456");

    LogEntry entry{};
    ctx.fill_tags(entry);

    EXPECT_TRUE(has_tag(entry, "env", "dev"));
    EXPECT_TRUE(has_tag(entry, "req", "456"));

    LogContext::pop_scoped_tag("req");
    ctx.remove_global_tag("env");
}

TEST(LogContext, ThreadNameSetGet) {
    LogContext::set_thread_name("worker");
    EXPECT_STREQ(LogContext::get_thread_name(), "worker");
    LogContext::set_thread_name("");
}

TEST(LogContext, ThreadIdCaching) {
    uint32_t first = LogContext::get_thread_id();
    uint32_t second = LogContext::get_thread_id();
    EXPECT_EQ(first, second);
    EXPECT_GT(first, 0u);
}

TEST(LogContext, FillThreadInfo) {
    LogContext::set_thread_name("io");
    LogEntry entry{};
    LogContext::instance().fill_thread_info(entry);

    EXPECT_EQ(entry.thread_id, LogContext::get_thread_id());
    EXPECT_STREQ(entry.thread_name, "io");
    EXPECT_GT(entry.process_id, 0u);

    LogContext::set_thread_name("");
}

TEST(LogContext, MultiThreadTLSIsolation) {
    std::array<LogEntry, 2> entries{};

    std::thread t1([&entries]() {
        LogContext::set_thread_name("t1");
        LogContext::push_scoped_tag("req", "111");
        LogContext::instance().fill_tags(entries[0]);
        LogContext::pop_scoped_tag("req");
    });

    std::thread t2([&entries]() {
        LogContext::set_thread_name("t2");
        LogContext::push_scoped_tag("req", "222");
        LogContext::instance().fill_tags(entries[1]);
        LogContext::pop_scoped_tag("req");
    });

    t1.join();
    t2.join();

    EXPECT_TRUE(has_tag(entries[0], "req", "111"));
    EXPECT_FALSE(has_tag(entries[0], "req", "222"));
    EXPECT_TRUE(has_tag(entries[1], "req", "222"));
    EXPECT_FALSE(has_tag(entries[1], "req", "111"));
}

TEST(LogContext, ProcessNameAndVersion) {
    LogContext& ctx = LogContext::instance();
    ctx.set_process_name("test_app");
    ctx.set_app_version("1.0.0");
    EXPECT_NE(ctx.git_hash(), nullptr);
    EXPECT_NE(ctx.build_type(), nullptr);
}

TEST(LogContext, GitHashAndBuildType) {
    LogContext& ctx = LogContext::instance();
    EXPECT_NE(ctx.git_hash(), nullptr);
    EXPECT_NE(ctx.build_type(), nullptr);
}
