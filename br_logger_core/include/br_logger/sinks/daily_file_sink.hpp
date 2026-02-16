#pragma once
#include <ctime>
#include <string>

#include "sink_interface.hpp"

namespace br_logger
{

class DailyFileSink : public ILogSink
{
 public:
  DailyFileSink(const std::string& base_dir, const std::string& base_name,
                size_t max_days = 0, bool use_utc = false);
  ~DailyFileSink();

  void Write(const LogEntry& entry) override;
  void Flush() override;

  std::string MakeFilename(std::time_t t) const;

 private:
  std::string base_dir_;
  std::string base_name_;
  size_t max_days_;
  bool use_utc_;
  int fd_;
  int current_day_;

  void OpenFileForToday();
  void CleanupOldFiles();
  int GetDay(std::time_t t) const;
  static void MkdirRecursive(const std::string& path);
};

}  // namespace br_logger
