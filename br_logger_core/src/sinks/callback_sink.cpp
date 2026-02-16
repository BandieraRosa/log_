#include "br_logger/sinks/callback_sink.hpp"

namespace br_logger
{

CallbackSink::CallbackSink(Callback cb) : callback_(std::move(cb)) {}

void CallbackSink::Write(const LogEntry& entry)
{
  if (!ShouldLog(entry.level))
  {
    return;
  }
  if (callback_)
  {
    callback_(entry);
  }
}

void CallbackSink::Flush() {}

}  // namespace br_logger
