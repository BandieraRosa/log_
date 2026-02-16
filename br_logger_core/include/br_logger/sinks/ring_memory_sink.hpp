#pragma once
#include "sink_interface.hpp"
#include <vector>
#include <cstddef>

namespace br_logger {

class RingMemorySink : public ILogSink {
public:
    explicit RingMemorySink(size_t capacity = 1024);

    void write(const LogEntry& entry) override;
    void flush() override;

    bool dump_to_file(const char* path);

    size_t size() const;

    const LogEntry& at(size_t index) const;

private:
    std::vector<LogEntry> buffer_;
    size_t capacity_;
    size_t head_;
    size_t count_;
};

} // namespace br_logger
