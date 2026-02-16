#pragma once
#include <cstddef>
#include <cstdint>

namespace br_logger
{

uint64_t monotonic_now_ns();
uint64_t wall_clock_now_ns();
size_t format_timestamp(uint64_t wall_ns, char* buf, size_t buf_size);
size_t format_date(uint64_t wall_ns, char* buf, size_t buf_size);
size_t format_time(uint64_t wall_ns, char* buf, size_t buf_size);

}  // namespace br_logger
