#include "br_logger/timestamp.hpp"
#include "br_logger/platform.hpp"
#include <cstdio>
#include <ctime>

#if BR_LOG_EMBEDDED

extern "C" __attribute__((weak)) uint64_t br_log_monotonic_ns() { return 0; }
extern "C" __attribute__((weak)) uint64_t br_log_wall_clock_ns() { return 0; }

namespace br_logger {

uint64_t monotonic_now_ns() { return br_log_monotonic_ns(); }
uint64_t wall_clock_now_ns() { return br_log_wall_clock_ns(); }

} // namespace br_logger

#elif defined(BR_LOG_PLATFORM_LINUX)

#include <time.h>

namespace br_logger {

uint64_t monotonic_now_ns() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
    return static_cast<uint64_t>(ts.tv_sec) * 1'000'000'000ULL
         + static_cast<uint64_t>(ts.tv_nsec);
}

uint64_t wall_clock_now_ns() {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return static_cast<uint64_t>(ts.tv_sec) * 1'000'000'000ULL
         + static_cast<uint64_t>(ts.tv_nsec);
}

} // namespace br_logger

#elif defined(BR_LOG_PLATFORM_MACOS)

#include <time.h>

namespace br_logger {

uint64_t monotonic_now_ns() {
    return clock_gettime_nsec_np(CLOCK_UPTIME_RAW);
}

uint64_t wall_clock_now_ns() {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return static_cast<uint64_t>(ts.tv_sec) * 1'000'000'000ULL
         + static_cast<uint64_t>(ts.tv_nsec);
}

} // namespace br_logger

#elif defined(BR_LOG_PLATFORM_WINDOWS)

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

namespace br_logger {

static uint64_t qpc_frequency() {
    LARGE_INTEGER freq;
    QueryPerformanceFrequency(&freq);
    return static_cast<uint64_t>(freq.QuadPart);
}

uint64_t monotonic_now_ns() {
    static const uint64_t freq = qpc_frequency();
    LARGE_INTEGER counter;
    QueryPerformanceCounter(&counter);
    uint64_t ticks = static_cast<uint64_t>(counter.QuadPart);
    return (ticks * 1'000'000'000ULL) / freq;
}

uint64_t wall_clock_now_ns() {
    FILETIME ft;
    GetSystemTimePreciseAsFileTime(&ft);
    uint64_t ticks = (static_cast<uint64_t>(ft.dwHighDateTime) << 32)
                   | static_cast<uint64_t>(ft.dwLowDateTime);
    // FILETIME epoch: 1601-01-01, Unix epoch offset: 11644473600 seconds
    constexpr uint64_t epoch_offset = 11644473600ULL * 10'000'000ULL;
    return (ticks - epoch_offset) * 100ULL;
}

} // namespace br_logger

#endif

namespace br_logger {

static void decompose_wall_ns(uint64_t wall_ns, struct tm& tm_out, uint32_t& us_out) {
    time_t sec = static_cast<time_t>(wall_ns / 1'000'000'000ULL);
    us_out = static_cast<uint32_t>((wall_ns % 1'000'000'000ULL) / 1'000ULL);
    localtime_r(&sec, &tm_out);
}

size_t format_timestamp(uint64_t wall_ns, char* buf, size_t buf_size) {
    if (buf_size == 0) return 0;
    struct tm tm_val{};
    uint32_t us;
    decompose_wall_ns(wall_ns, tm_val, us);
    int n = snprintf(buf, buf_size, "%04d-%02d-%02d %02d:%02d:%02d.%06u",
                     tm_val.tm_year + 1900, tm_val.tm_mon + 1, tm_val.tm_mday,
                     tm_val.tm_hour, tm_val.tm_min, tm_val.tm_sec, us);
    return (n > 0 && static_cast<size_t>(n) < buf_size)
         ? static_cast<size_t>(n) : (buf_size - 1);
}

size_t format_date(uint64_t wall_ns, char* buf, size_t buf_size) {
    if (buf_size == 0) return 0;
    struct tm tm_val{};
    uint32_t us;
    decompose_wall_ns(wall_ns, tm_val, us);
    int n = snprintf(buf, buf_size, "%04d-%02d-%02d",
                     tm_val.tm_year + 1900, tm_val.tm_mon + 1, tm_val.tm_mday);
    return (n > 0 && static_cast<size_t>(n) < buf_size)
         ? static_cast<size_t>(n) : (buf_size - 1);
}

size_t format_time(uint64_t wall_ns, char* buf, size_t buf_size) {
    if (buf_size == 0) return 0;
    struct tm tm_val{};
    uint32_t us;
    decompose_wall_ns(wall_ns, tm_val, us);
    int n = snprintf(buf, buf_size, "%02d:%02d:%02d.%06u",
                     tm_val.tm_hour, tm_val.tm_min, tm_val.tm_sec, us);
    return (n > 0 && static_cast<size_t>(n) < buf_size)
         ? static_cast<size_t>(n) : (buf_size - 1);
}

} // namespace br_logger
