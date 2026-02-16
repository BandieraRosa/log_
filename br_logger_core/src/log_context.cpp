#include "br_logger/log_context.hpp"

#include <cstring>
#include <mutex>
#include <shared_mutex>
#include <thread>

#include "br_logger/platform.hpp"

#if defined(BR_LOG_PLATFORM_LINUX)
#include <sys/syscall.h>
#include <unistd.h>
#elif defined(BR_LOG_PLATFORM_MACOS)
#include <pthread.h>
#include <unistd.h>
#elif defined(BR_LOG_PLATFORM_WINDOWS)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

namespace br_logger
{

thread_local FixedVector<LogTag, BR_LOG_MAX_TAGS> LogContext::tls_tags_{};
thread_local char LogContext::tls_thread_name_[32] = {};
thread_local uint32_t LogContext::tls_thread_id_ = 0;
thread_local bool LogContext::tls_thread_id_cached_ = false;

LogContext& LogContext::Instance()
{
  static LogContext ctx;
  return ctx;
}

void LogContext::SetGlobalTag(const char* key, const char* value)
{
  if (!key || !value) return;
  std::unique_lock<std::shared_mutex> lock(global_mutex_);
  for (size_t i = 0; i < global_tags_.Size(); ++i)
  {
    if (std::strncmp(global_tags_[i].key, key, BR_LOG_MAX_TAG_KEY_LEN) == 0)
    {
      std::strncpy(global_tags_[i].value, value, BR_LOG_MAX_TAG_VAL_LEN - 1);
      global_tags_[i].value[BR_LOG_MAX_TAG_VAL_LEN - 1] = '\0';
      return;
    }
  }

  LogTag tag{};
  std::strncpy(tag.key, key, BR_LOG_MAX_TAG_KEY_LEN - 1);
  tag.key[BR_LOG_MAX_TAG_KEY_LEN - 1] = '\0';
  std::strncpy(tag.value, value, BR_LOG_MAX_TAG_VAL_LEN - 1);
  tag.value[BR_LOG_MAX_TAG_VAL_LEN - 1] = '\0';
  global_tags_.PushBack(tag);
}

void LogContext::RemoveGlobalTag(const char* key)
{
  if (!key) return;
  std::unique_lock<std::shared_mutex> lock(global_mutex_);
  for (size_t i = 0; i < global_tags_.Size(); ++i)
  {
    if (std::strncmp(global_tags_[i].key, key, BR_LOG_MAX_TAG_KEY_LEN) == 0)
    {
      size_t last = global_tags_.Size() - 1;
      if (i != last)
      {
        global_tags_[i] = global_tags_[last];
      }
      global_tags_.PopBack();
      return;
    }
  }
}

void LogContext::SetProcessName(const char* name)
{
  if (!name) return;
  std::strncpy(process_name_, name, sizeof(process_name_) - 1);
  process_name_[sizeof(process_name_) - 1] = '\0';
}

void LogContext::SetAppVersion(const char* version)
{
  if (!version) return;
  std::strncpy(app_version_, version, sizeof(app_version_) - 1);
  app_version_[sizeof(app_version_) - 1] = '\0';
}

const char* LogContext::GitHash() const { return BR_LOG_GIT_HASH; }

const char* LogContext::BuildType() const { return BR_LOG_BUILD_TYPE; }

void LogContext::SetThreadName(const char* name)
{
  if (!name) return;
  std::strncpy(tls_thread_name_, name, sizeof(tls_thread_name_) - 1);
  tls_thread_name_[sizeof(tls_thread_name_) - 1] = '\0';
}

const char* LogContext::GetThreadName() { return tls_thread_name_; }

uint32_t LogContext::GetThreadId()
{
  if (!tls_thread_id_cached_)
  {
#if defined(BR_LOG_PLATFORM_LINUX)
    tls_thread_id_ = static_cast<uint32_t>(syscall(SYS_gettid));
#elif defined(BR_LOG_PLATFORM_MACOS)
    uint64_t tid = 0;
    pthread_threadid_np(nullptr, &tid);
    tls_thread_id_ = static_cast<uint32_t>(tid);
#elif defined(BR_LOG_PLATFORM_WINDOWS)
    tls_thread_id_ = static_cast<uint32_t>(GetCurrentThreadId());
#else
    tls_thread_id_ =
        static_cast<uint32_t>(std::hash<std::thread::id>{}(std::this_thread::get_id()));
#endif
    tls_thread_id_cached_ = true;
  }
  return tls_thread_id_;
}

void LogContext::PushScopedTag(const char* key, const char* value)
{
  if (!key || !value) return;
  LogTag tag{};
  std::strncpy(tag.key, key, BR_LOG_MAX_TAG_KEY_LEN - 1);
  tag.key[BR_LOG_MAX_TAG_KEY_LEN - 1] = '\0';
  std::strncpy(tag.value, value, BR_LOG_MAX_TAG_VAL_LEN - 1);
  tag.value[BR_LOG_MAX_TAG_VAL_LEN - 1] = '\0';
  tls_tags_.PushBack(tag);
}

void LogContext::PopScopedTag(const char* key)
{
  if (!key || tls_tags_.Empty()) return;
  for (size_t i = tls_tags_.Size(); i-- > 0;)
  {
    if (std::strncmp(tls_tags_[i].key, key, BR_LOG_MAX_TAG_KEY_LEN) == 0)
    {
      size_t last = tls_tags_.Size() - 1;
      if (i != last)
      {
        tls_tags_[i] = tls_tags_[last];
      }
      tls_tags_.PopBack();
      return;
    }
  }
}

void LogContext::FillTags(LogEntry& entry) const
{
  size_t count = 0;
  {
    std::shared_lock<std::shared_mutex> lock(global_mutex_);
    for (const auto& tag : global_tags_)
    {
      if (count >= BR_LOG_MAX_TAGS) break;
      entry.tags[count++] = tag;
    }
  }

  for (const auto& tag : tls_tags_)
  {
    if (count >= BR_LOG_MAX_TAGS) break;
    entry.tags[count++] = tag;
  }

  entry.tag_count = static_cast<uint8_t>(count);
}

void LogContext::FillThreadInfo(LogEntry& entry) const
{
#if defined(BR_LOG_PLATFORM_WINDOWS)
  static const uint32_t process_id = static_cast<uint32_t>(GetCurrentProcessId());
#elif defined(BR_LOG_PLATFORM_LINUX) || defined(BR_LOG_PLATFORM_MACOS)
  static const uint32_t process_id = static_cast<uint32_t>(getpid());
#else
  static const uint32_t process_id = 0;
#endif

  entry.process_id = process_id;
  entry.thread_id = GetThreadId();
  std::memcpy(entry.thread_name, tls_thread_name_, sizeof(entry.thread_name));
}

LogContext::ScopedTag::ScopedTag(const char* key, const char* value) : key_(key)
{
  LogContext::PushScopedTag(key, value);
}

LogContext::ScopedTag::~ScopedTag() { LogContext::PopScopedTag(key_); }

}  // namespace br_logger
