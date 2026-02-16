#include "../include/br_logger/backend.hpp"
#include "../include/br_logger/sinks/callback_sink.hpp"
#include "../include/br_logger/log_entry.hpp"
#include "../include/br_logger/log_level.hpp"
#include <gtest/gtest.h>
#include <atomic>
#include <cstring>
#include <memory>
#include <string>
#include <vector>

#if BR_LOG_HAS_THREAD
#include <thread>
#include <chrono>
#endif

static br_logger::LogEntry make_test_entry(
    br_logger::LogLevel level = br_logger::LogLevel::Info,
    const char* msg = "test message") {
    br_logger::LogEntry entry{};
    entry.level = level;
    entry.file_name = "test.cpp";
    entry.file_path = "test.cpp";
    entry.function_name = "test_func";
    entry.pretty_function = "void test_func()";
    entry.line = 1;
    entry.column = 0;
    entry.thread_id = 1;
    entry.process_id = 1;
    entry.tag_count = 0;
    entry.sequence_id = 0;
    size_t len = std::strlen(msg);
    if (len >= BR_LOG_MAX_MSG_LEN) len = BR_LOG_MAX_MSG_LEN - 1;
    std::memcpy(entry.msg, msg, len);
    entry.msg[len] = '\0';
    entry.msg_len = static_cast<uint16_t>(len);
    return entry;
}

TEST(LoggerBackend, PushAndDrain) {
    auto backend = std::make_unique<br_logger::LoggerBackend>();
    std::vector<std::string> received;
    auto sink = std::make_unique<br_logger::CallbackSink>(
        [&](const br_logger::LogEntry& e) {
            received.emplace_back(e.msg, e.msg_len);
        });
    backend->add_sink(std::move(sink));

    backend->try_push(make_test_entry(br_logger::LogLevel::Info, "hello"));
    backend->try_push(make_test_entry(br_logger::LogLevel::Warn, "world"));

    size_t drained = backend->drain();
    EXPECT_EQ(drained, 2u);
    ASSERT_EQ(received.size(), 2u);
    EXPECT_EQ(received[0], "hello");
    EXPECT_EQ(received[1], "world");
}

TEST(LoggerBackend, DrainMaxEntries) {
    auto backend = std::make_unique<br_logger::LoggerBackend>();
    std::atomic<int> count{0};
    auto sink = std::make_unique<br_logger::CallbackSink>(
        [&](const br_logger::LogEntry&) {
            count.fetch_add(1, std::memory_order_relaxed);
        });
    backend->add_sink(std::move(sink));

    for (int i = 0; i < 100; ++i) {
        ASSERT_TRUE(backend->try_push(make_test_entry()));
    }

    size_t first_batch = backend->drain(10);
    EXPECT_EQ(first_batch, 10u);
    EXPECT_EQ(count.load(), 10);

    size_t rest = 0;
    size_t batch;
    while ((batch = backend->drain(64)) > 0) {
        rest += batch;
    }
    EXPECT_EQ(rest, 90u);
    EXPECT_EQ(count.load(), 100);
}

TEST(LoggerBackend, StartStop) {
    auto backend = std::make_unique<br_logger::LoggerBackend>();
    std::atomic<int> count{0};
    auto sink = std::make_unique<br_logger::CallbackSink>(
        [&](const br_logger::LogEntry&) {
            count.fetch_add(1, std::memory_order_relaxed);
        });
    backend->add_sink(std::move(sink));

    backend->start();
    for (int i = 0; i < 50; ++i) {
        backend->try_push(make_test_entry());
    }
    backend->stop();

    EXPECT_EQ(count.load(), 50);
}

TEST(LoggerBackend, DispatchToMultipleSinks) {
    auto backend = std::make_unique<br_logger::LoggerBackend>();
    std::atomic<int> count1{0};
    std::atomic<int> count2{0};

    auto sink1 = std::make_unique<br_logger::CallbackSink>(
        [&](const br_logger::LogEntry&) {
            count1.fetch_add(1, std::memory_order_relaxed);
        });
    auto sink2 = std::make_unique<br_logger::CallbackSink>(
        [&](const br_logger::LogEntry&) {
            count2.fetch_add(1, std::memory_order_relaxed);
        });
    backend->add_sink(std::move(sink1));
    backend->add_sink(std::move(sink2));

    backend->try_push(make_test_entry());
    backend->try_push(make_test_entry());
    backend->drain();

    EXPECT_EQ(count1.load(), 2);
    EXPECT_EQ(count2.load(), 2);
}

TEST(LoggerBackend, RingFull) {
    auto backend = std::make_unique<br_logger::LoggerBackend>();
    auto sink = std::make_unique<br_logger::CallbackSink>(
        [](const br_logger::LogEntry&) {});
    backend->add_sink(std::move(sink));

    bool push_failed = false;
    for (size_t i = 0; i < BR_LOG_RING_SIZE + 10; ++i) {
        if (!backend->try_push(make_test_entry())) {
            push_failed = true;
            break;
        }
    }
    EXPECT_TRUE(push_failed);
}

TEST(LoggerBackend, StopFlushes) {
    auto backend = std::make_unique<br_logger::LoggerBackend>();
    std::atomic<int> count{0};
    auto sink = std::make_unique<br_logger::CallbackSink>(
        [&](const br_logger::LogEntry&) {
            count.fetch_add(1, std::memory_order_relaxed);
        });
    backend->add_sink(std::move(sink));

    for (int i = 0; i < 20; ++i) {
        backend->try_push(make_test_entry());
    }

    backend->stop();
    EXPECT_EQ(count.load(), 20);
}

TEST(LoggerBackend, DoubleStartStop) {
    auto backend = std::make_unique<br_logger::LoggerBackend>();
    auto sink = std::make_unique<br_logger::CallbackSink>(
        [](const br_logger::LogEntry&) {});
    backend->add_sink(std::move(sink));

    EXPECT_NO_THROW(backend->start());
    EXPECT_NO_THROW(backend->start());
    EXPECT_NO_THROW(backend->stop());
    EXPECT_NO_THROW(backend->stop());
}

TEST(LoggerBackend, DrainEmpty) {
    auto backend = std::make_unique<br_logger::LoggerBackend>();
    auto sink = std::make_unique<br_logger::CallbackSink>(
        [](const br_logger::LogEntry&) {});
    backend->add_sink(std::move(sink));

    EXPECT_EQ(backend->drain(), 0u);
}

#if BR_LOG_HAS_THREAD
TEST(LoggerBackend, WorkerThreadConsumes) {
    auto backend = std::make_unique<br_logger::LoggerBackend>();
    std::atomic<int> count{0};
    auto sink = std::make_unique<br_logger::CallbackSink>(
        [&](const br_logger::LogEntry&) {
            count.fetch_add(1, std::memory_order_relaxed);
        });
    backend->add_sink(std::move(sink));

    backend->start();
    for (int i = 0; i < 30; ++i) {
        backend->try_push(make_test_entry());
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    EXPECT_EQ(count.load(), 30);
    backend->stop();
}
#endif
