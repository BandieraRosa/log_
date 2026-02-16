#include "br_logger/sinks/callback_sink.hpp"

namespace br_logger {

CallbackSink::CallbackSink(Callback cb)
    : callback_(std::move(cb)) {}

void CallbackSink::write(const LogEntry& entry) {
    if (!should_log(entry.level)) {
        return;
    }
    if (callback_) {
        callback_(entry);
    }
}

void CallbackSink::flush() {
}

} // namespace br_logger
