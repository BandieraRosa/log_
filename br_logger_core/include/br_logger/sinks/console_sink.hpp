#pragma once
#include <optional>

#include "sink_interface.hpp"

namespace br_logger
{

class ConsoleSink : public ILogSink
{
 public:
  explicit ConsoleSink(std::optional<bool> force_color = std::nullopt);

  void Write(const LogEntry& entry) override;
  void Flush() override;

 private:
  bool use_color_;
  bool stdout_is_tty_;
  bool stderr_is_tty_;
};

}  // namespace br_logger
