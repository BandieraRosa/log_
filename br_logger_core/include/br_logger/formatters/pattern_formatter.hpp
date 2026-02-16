#pragma once
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include "formatter_interface.hpp"

namespace br_logger
{

class PatternFormatter : public IFormatter
{
 public:
  explicit PatternFormatter(
      std::string_view pattern = "[%D %T%e] [%C%L%R] [tid:%t] [%f:%#::%n] %g %m",
      bool enable_color = true);

  size_t Format(const LogEntry& entry, char* buf, size_t buf_size) override;

 private:
  std::string pattern_;
  bool enable_color_;

  enum class OpType : uint8_t
  {
    Literal,
    Date,
    Time,
    Microseconds,
    LevelFull,
    LevelShort,
    FileName,
    FilePath,
    FuncName,
    PrettyFunc,
    Line,
    ThreadId,
    ProcessId,
    ThreadName,
    SequenceId,
    Tags,
    Message,
    ColorStart,
    ColorReset
  };

  struct FormatOp
  {
    OpType type;
    std::string literal;
  };

  std::vector<FormatOp> ops_;
  void CompilePattern();
};

}  // namespace br_logger
