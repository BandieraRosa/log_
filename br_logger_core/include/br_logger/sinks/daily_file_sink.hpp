#pragma once
#include "sink_interface.hpp"
#include <string>
#include <ctime>

namespace br_logger {

class DailyFileSink : public ILogSink {
public:
    DailyFileSink(const std::string& base_dir, const std::string& base_name,
                  size_t max_days = 0, bool use_utc = false);
    ~DailyFileSink();

    void write(const LogEntry& entry) override;
    void flush() override;

    std::string make_filename(std::time_t t) const;

private:
    std::string base_dir_;
    std::string base_name_;
    size_t max_days_;
    bool use_utc_;
    int fd_;
    int current_day_;

    void open_file_for_today();
    void cleanup_old_files();
    int get_day(std::time_t t) const;
    static void mkdir_recursive(const std::string& path);
};

} // namespace br_logger
