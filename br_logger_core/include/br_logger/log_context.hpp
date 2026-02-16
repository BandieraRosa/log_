#pragma once
#include <cstdint>
#include <shared_mutex>

#include "fixed_vector.hpp"
#include "log_entry.hpp"

namespace br_logger
{

// NOLINTBEGIN(readability-identifier-naming)
class LogContext
{
 public:
  static LogContext& Instance();

  void SetGlobalTag(const char* key, const char* value);
  void RemoveGlobalTag(const char* key);

  void SetProcessName(const char* name);
  void SetAppVersion(const char* version);
  const char* GitHash() const;
  const char* BuildType() const;

  static void SetThreadName(const char* name);
  static const char* GetThreadName();
  static uint32_t GetThreadId();

  static void PushScopedTag(const char* key, const char* value);
  static void PopScopedTag(const char* key);

  void FillTags(LogEntry& entry) const;
  void FillThreadInfo(LogEntry& entry) const;

  class ScopedTag
  {
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

  mutable std::shared_mutex global_mutex_;
  FixedVector<LogTag, 16> global_tags_;

  char process_name_[64] = {};
  char app_version_[32] = {};

  static thread_local FixedVector<LogTag, BR_LOG_MAX_TAGS> tls_tags_;
  static thread_local char tls_thread_name_[32];
  static thread_local uint32_t tls_thread_id_;
  static thread_local bool tls_thread_id_cached_;
};
// NOLINTEND(readability-identifier-naming)

#define BR_LOG_CONCAT_IMPL_(a, b) a##b
#define BR_LOG_CONCAT_(a, b) BR_LOG_CONCAT_IMPL_(a, b)
#define LOG_SCOPED_TAG(key, value)                                                     \
  ::br_logger::LogContext::ScopedTag BR_LOG_CONCAT_(_br_scoped_tag_, __COUNTER__)(key, \
                                                                                  value)

}  // namespace br_logger
