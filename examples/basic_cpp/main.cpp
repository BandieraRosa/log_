#include <br_logger/formatters/json_formatter.hpp>
#include <br_logger/formatters/pattern_formatter.hpp>
#include <br_logger/log_context.hpp>
#include <br_logger/logger.hpp>
#include <br_logger/sinks/callback_sink.hpp>
#include <br_logger/sinks/console_sink.hpp>
#include <br_logger/sinks/rotating_file_sink.hpp>
#include <cstdio>
#include <thread>

int main()
{
  auto& logger = br_logger::Logger::Instance();

  // --- Sink setup ---

  // 1) Console sink with pattern formatter
  auto console = std::make_unique<br_logger::ConsoleSink>();
  console->SetFormatter(std::make_unique<br_logger::PatternFormatter>(
      "[%D %T.%e] [%L] [tid:%t] [%f:%#] %g %m"));
  logger.AddSink(std::move(console));

  // 2) Rotating file sink with JSON formatter
  auto file_sink = std::make_unique<br_logger::RotatingFileSink>(
      "/tmp/br_logger_example.log", 1024 * 1024, 3);
  file_sink->SetFormatter(std::make_unique<br_logger::JsonFormatter>());
  logger.AddSink(std::move(file_sink));

  // 3) Callback sink (custom processing)
  auto cb_sink = std::make_unique<br_logger::CallbackSink>(
      [](const br_logger::LogEntry& entry)
      {
        if (entry.level >= br_logger::LogLevel::Error)
        {
          std::fprintf(stderr, "[ALERT] %s\n", entry.msg);
        }
      });
  logger.AddSink(std::move(cb_sink));

  // --- Configuration ---

  logger.SetLevel(br_logger::LogLevel::Trace);
  br_logger::LogContext::Instance().SetProcessName("basic_example");
  br_logger::LogContext::Instance().SetAppVersion("1.0.0");
  br_logger::LogContext::SetThreadName("main");

  // Global tag visible to all threads
  br_logger::LogContext::Instance().SetGlobalTag("env", "dev");

  // Start backend consumer thread
  logger.Start();

  // --- Basic logging ---

  LOG_TRACE("application started");
  LOG_DEBUG("debug value: %d", 42);
  LOG_INFO("hello %s, version %s", "world", "1.0");
  LOG_WARN("disk usage at %d%%", 85);
  LOG_ERROR("connection failed: %s", "timeout");

  // --- ScopedTag (RAII, thread-local) ---
  {
    br_logger::LogContext::ScopedTag tag("module", "network");
    LOG_INFO("sending request to %s", "api.example.com");
    LOG_INFO("received %d bytes", 4096);
  }  // "module" tag removed here
  LOG_INFO("back to main context");

  // --- Conditional logging ---

  int error_code = 404;
  LOG_WARN_IF(error_code != 200, "HTTP error: %d", error_code);
  LOG_ERROR_IF(error_code >= 500, "server error: %d", error_code);

  // --- Rate-limited logging ---

  for (int i = 0; i < 100; ++i)
  {
    LOG_EVERY_N(Info, 25, "progress: iteration %d", i);
  }

  // --- LOG_ONCE ---

  for (int i = 0; i < 10; ++i)
  {
    LOG_ONCE(Warn, "this warning only appears once");
  }

  // --- Multi-thread demo ---

  auto worker = [](int id)
  {
    char name[16];
    std::snprintf(name, sizeof(name), "worker-%d", id);
    br_logger::LogContext::SetThreadName(name);

    br_logger::LogContext::ScopedTag tag("worker_id", name);
    for (int i = 0; i < 5; ++i)
    {
      LOG_INFO("task %d processing step %d", id, i);
    }
  };

  std::thread t1(worker, 1);
  std::thread t2(worker, 2);
  t1.join();
  t2.join();

  // --- Shutdown ---

  LOG_INFO("shutting down");
  logger.Stop();

  std::printf("Example finished. Check /tmp/br_logger_example.log for JSON output.\n");
  return 0;
}
