#include "br_logger/sinks/console_sink.hpp"

#include <cstdio>

#include "br_logger/formatters/pattern_formatter.hpp"

#if defined(BR_LOG_PLATFORM_LINUX) || defined(BR_LOG_PLATFORM_MACOS)
#include <unistd.h>
#elif defined(BR_LOG_PLATFORM_WINDOWS)
#include <io.h>
#define isatty _isatty
#define STDOUT_FILENO 1
#define STDERR_FILENO 2
#endif

namespace br_logger
{

ConsoleSink::ConsoleSink(std::optional<bool> force_color)
{
  stdout_is_tty_ = ::isatty(STDOUT_FILENO) != 0;
  stderr_is_tty_ = ::isatty(STDERR_FILENO) != 0;

  if (force_color.has_value())
  {
    use_color_ = force_color.value();
  }
  else
  {
    use_color_ = stdout_is_tty_ || stderr_is_tty_;
  }
}

void ConsoleSink::Write(const LogEntry& entry)
{
  if (!ShouldLog(entry.level))
  {
    return;
  }

  if (!formatter_)
  {
    formatter_ = std::make_unique<PatternFormatter>(
        "[%D %T%e] [%C%L%R] [tid:%t] [%f:%#::%n] %g %m", use_color_);
  }

  size_t len = DoFormat(entry);
  if (len == 0)
  {
    return;
  }

  FILE* target = (entry.level >= LogLevel::WARN) ? stderr : stdout;
  std::fwrite(format_buf_, 1, len, target);
  std::fwrite("\n", 1, 1, target);
}

void ConsoleSink::Flush()
{
  std::fflush(stdout);
  std::fflush(stderr);
}

}  // namespace br_logger
