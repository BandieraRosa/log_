#pragma once
#include "formatter_interface.hpp"

namespace br_logger {

class JsonFormatter : public IFormatter {
public:
    explicit JsonFormatter(bool pretty = false);
    size_t format(const LogEntry& entry, char* buf, size_t buf_size) override;

private:
    bool pretty_;
    static size_t EscapeJsonString(const char* src, size_t src_len, char* dst, size_t dst_size);
};

} // namespace br_logger
