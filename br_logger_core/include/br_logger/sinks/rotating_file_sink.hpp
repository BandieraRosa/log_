#pragma once
#include <string>

#include "sink_interface.hpp"

namespace br_logger
{

class RotatingFileSink : public ILogSink
{
 public:
  // base_path: e.g. "/var/log/app.log"
  // max_file_size: max bytes per file before rotation
  // max_files: number of rotated files to keep (e.g. 3 means app.log, app.1.log,
  // app.2.log, app.3.log)
  RotatingFileSink(const std::string& base_path, size_t max_file_size,
                   size_t max_files = 5);
  ~RotatingFileSink();

  void Write(const LogEntry& entry) override;
  void Flush() override;

 private:
  std::string base_path_;
  size_t max_file_size_;
  size_t max_files_;
  size_t current_size_;
  int fd_;

  void OpenFile();
  void Rotate();
};

}  // namespace br_logger
