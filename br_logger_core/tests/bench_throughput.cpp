#include <benchmark/benchmark.h>

#include <algorithm>
#include <br_logger/formatters/pattern_formatter.hpp>
#include <br_logger/log_context.hpp>
#include <br_logger/logger.hpp>
#include <br_logger/sinks/callback_sink.hpp>
#include <br_logger/sinks/sink_interface.hpp>
#include <chrono>
#include <vector>

namespace
{

class NullSink : public br_logger::ILogSink
{
 public:
  void Write(const br_logger::LogEntry&) override {}
  void Flush() override {}
};

void setup_logger_with_null_sink()
{
  auto& logger = br_logger::Logger::Instance();
  logger.SetLevel(br_logger::LogLevel::TRACE);
  auto sink = std::make_unique<NullSink>();
  sink->SetFormatter(std::make_unique<br_logger::PatternFormatter>());
  logger.AddSink(std::move(sink));
  logger.Start();
}

void teardown_logger() { br_logger::Logger::Instance().Stop(); }

}  // namespace

static void bm_single_thread_log_info(benchmark::State& state)
{
  setup_logger_with_null_sink();
  int i = 0;
  for (auto _ : state)
  {
    LOG_INFO("benchmark message %d", i++);
  }
  state.SetItemsProcessed(state.iterations());
  teardown_logger();
}
BENCHMARK(bm_single_thread_log_info);

static void bm_multi_thread_log_info(benchmark::State& state)
{
  if (state.thread_index() == 0)
  {
    setup_logger_with_null_sink();
  }

  int i = 0;
  for (auto _ : state)
  {
    LOG_INFO("thread %d msg %d", static_cast<int>(state.thread_index()), i++);
  }
  state.SetItemsProcessed(state.iterations());

  if (state.thread_index() == 0)
  {
    teardown_logger();
  }
}
BENCHMARK(bm_multi_thread_log_info)->Threads(4);

static void bm_compile_time_filtered(benchmark::State& state)
{
  for (auto _ : state)
  {
    benchmark::DoNotOptimize(0);
  }
  state.SetItemsProcessed(state.iterations());
}
BENCHMARK(bm_compile_time_filtered);

static void bm_runtime_filtered(benchmark::State& state)
{
  setup_logger_with_null_sink();
  br_logger::Logger::Instance().SetLevel(br_logger::LogLevel::ERROR);

  for (auto _ : state)
  {
    LOG_DEBUG("this should be filtered at runtime %d", 42);
  }
  state.SetItemsProcessed(state.iterations());
  teardown_logger();
}
BENCHMARK(bm_runtime_filtered);

static void bm_p99_latency(benchmark::State& state)
{
  setup_logger_with_null_sink();

  std::vector<int64_t> latencies;
  latencies.reserve(1000000);

  for (auto _ : state)
  {
    auto start = std::chrono::high_resolution_clock::now();
    LOG_INFO("latency test %d", 123);
    auto end = std::chrono::high_resolution_clock::now();
    latencies.push_back(
        std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count());
  }

  if (!latencies.empty())
  {
    std::sort(latencies.begin(), latencies.end());
    size_t p99_idx = static_cast<size_t>(static_cast<double>(latencies.size()) * 0.99);
    if (p99_idx >= latencies.size())
    {
      p99_idx = latencies.size() - 1;
    }
    state.counters["p99_ns"] = static_cast<double>(latencies[p99_idx]);
    state.counters["p50_ns"] = static_cast<double>(latencies[latencies.size() / 2]);
    state.counters["max_ns"] = static_cast<double>(latencies.back());
  }

  state.SetItemsProcessed(state.iterations());
  teardown_logger();
}
BENCHMARK(bm_p99_latency)->Iterations(100000);

BENCHMARK_MAIN();
