#pragma once
#include "../log_entry.hpp"
#include <cstddef>

namespace br_logger {


class IFormatter {
public:
    virtual ~IFormatter() = default;
    virtual size_t format(const LogEntry& entry, char* buf, size_t buf_size) = 0;
};


} // namespace br_logger
