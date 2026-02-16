#pragma once
#include <functional>

#include "sink_interface.hpp"

namespace br_logger
{

class CallbackSink : public ILogSink
{
 public:
  using Callback = std::function<void(const LogEntry&)>;

  explicit CallbackSink(Callback cb);

  void Write(const LogEntry& entry) override;
  void Flush() override;

 private:
  Callback callback_;
};

}  // namespace br_logger
