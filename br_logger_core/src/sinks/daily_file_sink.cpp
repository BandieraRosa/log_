#include "br_logger/sinks/daily_file_sink.hpp"
#include "br_logger/formatters/pattern_formatter.hpp"

#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>
#include <cstring>
#include <cstdio>
#include <ctime>
#include <string>

namespace br_logger {

void DailyFileSink::mkdir_recursive(const std::string& path) {
    std::string tmp;
    for (size_t i = 0; i < path.size(); ++i) {
        tmp += path[i];
        if (path[i] == '/' || i == path.size() - 1) {
            ::mkdir(tmp.c_str(), 0755);
        }
    }
}

DailyFileSink::DailyFileSink(const std::string& base_dir, const std::string& base_name,
                             size_t max_days, bool use_utc)
    : base_dir_(base_dir)
    , base_name_(base_name)
    , max_days_(max_days)
    , use_utc_(use_utc)
    , fd_(-1)
    , current_day_(-1)
{
    mkdir_recursive(base_dir_);
    open_file_for_today();
}

DailyFileSink::~DailyFileSink() {
    if (fd_ >= 0) {
        ::fsync(fd_);
        ::close(fd_);
        fd_ = -1;
    }
}

int DailyFileSink::get_day(std::time_t t) const {
    std::tm tm_buf{};
    if (use_utc_) {
        ::gmtime_r(&t, &tm_buf);
    } else {
        ::localtime_r(&t, &tm_buf);
    }
    return tm_buf.tm_yday + tm_buf.tm_year * 366;
}

std::string DailyFileSink::make_filename(std::time_t t) const {
    std::tm tm_buf{};
    if (use_utc_) {
        ::gmtime_r(&t, &tm_buf);
    } else {
        ::localtime_r(&t, &tm_buf);
    }
    char date_str[16];
    std::snprintf(date_str, sizeof(date_str), "%04d-%02d-%02d",
                  tm_buf.tm_year + 1900, tm_buf.tm_mon + 1, tm_buf.tm_mday);

    std::string result = base_dir_;
    if (!result.empty() && result.back() != '/') {
        result += '/';
    }
    result += base_name_;
    result += '_';
    result += date_str;
    result += ".log";
    return result;
}

void DailyFileSink::open_file_for_today() {
    std::time_t now = std::time(nullptr);
    std::string filename = make_filename(now);

    if (fd_ >= 0) {
        ::fsync(fd_);
        ::close(fd_);
    }

    fd_ = ::open(filename.c_str(), O_WRONLY | O_CREAT | O_APPEND, 0644);
    current_day_ = get_day(now);

    if (max_days_ > 0) {
        cleanup_old_files();
    }
}

void DailyFileSink::cleanup_old_files() {
    DIR* dir = ::opendir(base_dir_.c_str());
    if (!dir) return;

    std::string prefix = base_name_ + "_";
    std::string suffix = ".log";

    std::time_t now = std::time(nullptr);
    double max_seconds = static_cast<double>(max_days_) * 86400.0;

    struct dirent* ent;
    while ((ent = ::readdir(dir)) != nullptr) {
        std::string name(ent->d_name);
        if (name.size() < prefix.size() + suffix.size()) continue;
        if (name.compare(0, prefix.size(), prefix) != 0) continue;
        if (name.compare(name.size() - suffix.size(), suffix.size(), suffix) != 0) continue;

        std::string full_path = base_dir_;
        if (!full_path.empty() && full_path.back() != '/') {
            full_path += '/';
        }
        full_path += name;

        struct stat st{};
        if (::stat(full_path.c_str(), &st) == 0) {
            double age = std::difftime(now, st.st_mtime);
            if (age > max_seconds) {
                ::unlink(full_path.c_str());
            }
        }
    }
    ::closedir(dir);
}

void DailyFileSink::write(const LogEntry& entry) {
    if (!should_log(entry.level)) {
        return;
    }

    if (!formatter_) {
        formatter_ = std::make_unique<PatternFormatter>(
            "[%D %T%e] [%C%L%R] [tid:%t] [%f:%#::%n] %g %m",
            false);
    }

    std::time_t now = std::time(nullptr);
    int today = get_day(now);
    if (today != current_day_) {
        open_file_for_today();
    }

    size_t len = do_format(entry);
    if (len == 0 || fd_ < 0) {
        return;
    }

    ::write(fd_, format_buf_, len);
    ::write(fd_, "\n", 1);
}

void DailyFileSink::flush() {
    if (fd_ >= 0) {
        ::fsync(fd_);
    }
}

} // namespace br_logger
