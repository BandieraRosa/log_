#pragma once
#include "sink_interface.hpp"
#include <optional>

namespace br_logger {

class ConsoleSink : public ILogSink {
public:
    explicit ConsoleSink(std::optional<bool> force_color = std::nullopt);

    void write(const LogEntry& entry) override;
    void flush() override;

private:
    bool use_color_;
    bool stdout_is_tty_;
    bool stderr_is_tty_;
};

} // namespace br_logger
