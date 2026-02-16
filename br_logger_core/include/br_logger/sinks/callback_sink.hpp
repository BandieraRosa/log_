#pragma once
#include "sink_interface.hpp"
#include <functional>

namespace br_logger {

class CallbackSink : public ILogSink {
public:
    using Callback = std::function<void(const LogEntry&)>;

    explicit CallbackSink(Callback cb);

    void write(const LogEntry& entry) override;
    void flush() override;

private:
    Callback callback_;
};

} // namespace br_logger
