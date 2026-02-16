#pragma once
#include "log_entry.hpp"
#include "fixed_vector.hpp"
#include <cstdint>
#include <mutex>

namespace br_logger {

// NOLINTBEGIN(readability-identifier-naming)
class LogContext {
public:
    static LogContext& instance();

    void set_global_tag(const char* key, const char* value);
    void remove_global_tag(const char* key);

    void set_process_name(const char* name);
    void set_app_version(const char* version);
    const char* git_hash() const;
    const char* build_type() const;

    static void set_thread_name(const char* name);
    static const char* get_thread_name();
    static uint32_t get_thread_id();

    static void push_scoped_tag(const char* key, const char* value);
    static void pop_scoped_tag(const char* key);

    void fill_tags(LogEntry& entry) const;
    void fill_thread_info(LogEntry& entry) const;

    class ScopedTag {
    public:
        ScopedTag(const char* key, const char* value);
        ~ScopedTag();
        ScopedTag(const ScopedTag&) = delete;
        ScopedTag& operator=(const ScopedTag&) = delete;
    private:
        const char* key_;
    };

private:
    LogContext() = default;

    mutable std::mutex global_mutex_;
    FixedVector<LogTag, 16> global_tags_;

    char process_name_[64] = {};
    char app_version_[32] = {};

    static thread_local FixedVector<LogTag, BR_LOG_MAX_TAGS> tls_tags_;
    static thread_local char tls_thread_name_[32];
    static thread_local uint32_t tls_thread_id_;
    static thread_local bool tls_thread_id_cached_;
};
// NOLINTEND(readability-identifier-naming)

#define LOG_SCOPED_TAG(key, value) \
    ::br_logger::LogContext::ScopedTag _hpc_scoped_tag_##__LINE__(key, value)

} // namespace br_logger
