#include <gtest/gtest.h>

#include <array>
#include <cstring>
#include <thread>

#include "br_logger/log_context.hpp"
#include "br_logger/log_entry.hpp"

using br_logger::LogContext;
using br_logger::LogEntry;

namespace
{

bool has_tag(const LogEntry& entry, const char* key, const char* value)
{
  for (uint8_t i = 0; i < entry.tag_count; ++i)
  {
    if (std::strcmp(entry.tags[i].key, key) == 0 &&
        std::strcmp(entry.tags[i].value, value) == 0)
    {
      return true;
    }
  }
  return false;
}

bool has_key(const LogEntry& entry, const char* key)
{
  for (uint8_t i = 0; i < entry.tag_count; ++i)
  {
    if (std::strcmp(entry.tags[i].key, key) == 0)
    {
      return true;
    }
  }
  return false;
}

}  // namespace

TEST(LogContext, GlobalTagSetAndFill)
{
  LogContext& ctx = LogContext::Instance();
  ctx.SetGlobalTag("env", "prod");

  LogEntry entry{};
  ctx.FillTags(entry);

  EXPECT_TRUE(has_tag(entry, "env", "prod"));

  ctx.RemoveGlobalTag("env");
}

TEST(LogContext, GlobalTagRemove)
{
  LogContext& ctx = LogContext::Instance();
  ctx.SetGlobalTag("stage", "blue");
  ctx.RemoveGlobalTag("stage");

  LogEntry entry{};
  ctx.FillTags(entry);

  EXPECT_FALSE(has_key(entry, "stage"));
}

TEST(LogContext, GlobalTagUpdate)
{
  LogContext& ctx = LogContext::Instance();
  ctx.SetGlobalTag("region", "us");
  ctx.SetGlobalTag("region", "eu");

  LogEntry entry{};
  ctx.FillTags(entry);

  EXPECT_EQ(entry.tag_count, 1);
  EXPECT_TRUE(has_tag(entry, "region", "eu"));

  ctx.RemoveGlobalTag("region");
}

TEST(LogContext, TLSTagPushAndFill)
{
  LogContext::PushScopedTag("req", "123");

  LogEntry entry{};
  LogContext::Instance().FillTags(entry);
  EXPECT_TRUE(has_tag(entry, "req", "123"));

  LogContext::PopScopedTag("req");
  LogEntry entry_after{};
  LogContext::Instance().FillTags(entry_after);
  EXPECT_FALSE(has_key(entry_after, "req"));
}

TEST(LogContext, ScopedTagRAII)
{
  LogEntry entry_inside{};
  {
    LogContext::ScopedTag scoped("trace", "abc");
    LogContext::Instance().FillTags(entry_inside);
    EXPECT_TRUE(has_tag(entry_inside, "trace", "abc"));
  }

  LogEntry entry_outside{};
  LogContext::Instance().FillTags(entry_outside);
  EXPECT_FALSE(has_key(entry_outside, "trace"));
}

TEST(LogContext, FillTagsCombinesGlobalAndTLS)
{
  LogContext& ctx = LogContext::Instance();
  ctx.SetGlobalTag("env", "dev");
  LogContext::PushScopedTag("req", "456");

  LogEntry entry{};
  ctx.FillTags(entry);

  EXPECT_TRUE(has_tag(entry, "env", "dev"));
  EXPECT_TRUE(has_tag(entry, "req", "456"));

  LogContext::PopScopedTag("req");
  ctx.RemoveGlobalTag("env");
}

TEST(LogContext, ThreadNameSetGet)
{
  LogContext::SetThreadName("worker");
  EXPECT_STREQ(LogContext::GetThreadName(), "worker");
  LogContext::SetThreadName("");
}

TEST(LogContext, ThreadIdCaching)
{
  uint32_t first = LogContext::GetThreadId();
  uint32_t second = LogContext::GetThreadId();
  EXPECT_EQ(first, second);
  EXPECT_GT(first, 0u);
}

TEST(LogContext, FillThreadInfo)
{
  LogContext::SetThreadName("io");
  LogEntry entry{};
  LogContext::Instance().FillThreadInfo(entry);

  EXPECT_EQ(entry.thread_id, LogContext::GetThreadId());
  EXPECT_STREQ(entry.thread_name, "io");
  EXPECT_GT(entry.process_id, 0u);

  LogContext::SetThreadName("");
}

TEST(LogContext, MultiThreadTLSIsolation)
{
  std::array<LogEntry, 2> entries{};

  std::thread t1(
      [&entries]()
      {
        LogContext::SetThreadName("t1");
        LogContext::PushScopedTag("req", "111");
        LogContext::Instance().FillTags(entries[0]);
        LogContext::PopScopedTag("req");
      });

  std::thread t2(
      [&entries]()
      {
        LogContext::SetThreadName("t2");
        LogContext::PushScopedTag("req", "222");
        LogContext::Instance().FillTags(entries[1]);
        LogContext::PopScopedTag("req");
      });

  t1.join();
  t2.join();

  EXPECT_TRUE(has_tag(entries[0], "req", "111"));
  EXPECT_FALSE(has_tag(entries[0], "req", "222"));
  EXPECT_TRUE(has_tag(entries[1], "req", "222"));
  EXPECT_FALSE(has_tag(entries[1], "req", "111"));
}

TEST(LogContext, ProcessNameAndVersion)
{
  LogContext& ctx = LogContext::Instance();
  ctx.SetProcessName("test_app");
  ctx.SetAppVersion("1.0.0");
  EXPECT_NE(ctx.GitHash(), nullptr);
  EXPECT_NE(ctx.BuildType(), nullptr);
}

TEST(LogContext, GitHashAndBuildType)
{
  LogContext& ctx = LogContext::Instance();
  EXPECT_NE(ctx.GitHash(), nullptr);
  EXPECT_NE(ctx.BuildType(), nullptr);
}
