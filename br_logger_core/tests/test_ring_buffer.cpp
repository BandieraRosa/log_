#include <gtest/gtest.h>

#include <array>
#include <atomic>
#include <cstddef>
#include <thread>
#include <unordered_set>
#include <vector>

#include "br_logger/ring_buffer.hpp"

using br_logger::MPSCRingBuffer;

struct TestItem
{
  uint32_t producer_id;
  uint32_t sequence;
};

namespace
{

constexpr uint32_t K_PRODUCER_COUNT = 4;
constexpr uint32_t K_ITEMS_PER_PRODUCER = 1000;
constexpr uint32_t K_STRESS_PRODUCERS = 8;
constexpr uint32_t K_STRESS_ITEMS = 5000;

uint64_t make_key(const TestItem& item)
{
  return (static_cast<uint64_t>(item.producer_id) << 32) | item.sequence;
}

}  // namespace

TEST(MPSCRingBuffer, SinglePushPop)
{
  MPSCRingBuffer<TestItem, 8> buffer;
  TestItem in{1, 42};
  TestItem out{};

  EXPECT_TRUE(buffer.TryPush(in));
  EXPECT_TRUE(buffer.TryPop(out));
  EXPECT_EQ(out.producer_id, in.producer_id);
  EXPECT_EQ(out.sequence, in.sequence);
}

TEST(MPSCRingBuffer, PushPopMultiple)
{
  MPSCRingBuffer<TestItem, 16> buffer;
  std::vector<TestItem> items;
  items.reserve(10);
  for (uint32_t i = 0; i < 10; ++i)
  {
    items.push_back({2, i});
    EXPECT_TRUE(buffer.TryPush(items.back()));
  }

  for (uint32_t i = 0; i < items.size(); ++i)
  {
    TestItem out{};
    EXPECT_TRUE(buffer.TryPop(out));
    EXPECT_EQ(out.producer_id, items[i].producer_id);
    EXPECT_EQ(out.sequence, items[i].sequence);
  }
}

TEST(MPSCRingBuffer, EmptyPopFails)
{
  MPSCRingBuffer<TestItem, 8> buffer;
  TestItem out{};
  EXPECT_FALSE(buffer.TryPop(out));
}

TEST(MPSCRingBuffer, FullPushFails)
{
  MPSCRingBuffer<TestItem, 4> buffer;
  EXPECT_TRUE(buffer.TryPush({1, 1}));
  EXPECT_TRUE(buffer.TryPush({1, 2}));
  EXPECT_TRUE(buffer.TryPush({1, 3}));
  EXPECT_TRUE(buffer.TryPush({1, 4}));
  EXPECT_FALSE(buffer.TryPush({1, 5}));
}

TEST(MPSCRingBuffer, CapacityCheck)
{
  MPSCRingBuffer<TestItem, 64> buffer;
  EXPECT_EQ(buffer.GetCapacity(), 64u);
}

TEST(MPSCRingBuffer, EmptyPredicate)
{
  MPSCRingBuffer<TestItem, 8> buffer;
  EXPECT_TRUE(buffer.Empty());

  EXPECT_TRUE(buffer.TryPush({0, 0}));
  EXPECT_FALSE(buffer.Empty());

  TestItem out{};
  EXPECT_TRUE(buffer.TryPop(out));
  EXPECT_TRUE(buffer.Empty());
}

TEST(MPSCRingBuffer, WrapAround)
{
  MPSCRingBuffer<TestItem, 8> buffer;

  for (uint32_t round = 0; round < 5; ++round)
  {
    for (uint32_t i = 0; i < 8; ++i)
    {
      EXPECT_TRUE(buffer.TryPush({round, i}));
    }
    for (uint32_t i = 0; i < 8; ++i)
    {
      TestItem out{};
      EXPECT_TRUE(buffer.TryPop(out));
      EXPECT_EQ(out.producer_id, round);
      EXPECT_EQ(out.sequence, i);
    }
  }
  EXPECT_TRUE(buffer.Empty());
}

TEST(MPSCRingBuffer, MultiProducerSingleConsumer)
{
  MPSCRingBuffer<TestItem, 1024> buffer;
  std::atomic<uint32_t> remaining{K_PRODUCER_COUNT * K_ITEMS_PER_PRODUCER};
  std::vector<std::thread> producers;
  producers.reserve(K_PRODUCER_COUNT);

  for (uint32_t producer = 0; producer < K_PRODUCER_COUNT; ++producer)
  {
    producers.emplace_back(
        [producer, &buffer]()
        {
          for (uint32_t i = 0; i < K_ITEMS_PER_PRODUCER; ++i)
          {
            TestItem item{producer, i};
            while (!buffer.TryPush(item))
            {
              std::this_thread::yield();
            }
          }
        });
  }

  std::array<uint32_t, K_PRODUCER_COUNT> last_sequence;
  last_sequence.fill(0);
  std::array<bool, K_PRODUCER_COUNT> seen_any;
  seen_any.fill(false);
  std::unordered_set<uint64_t> seen;
  seen.reserve(static_cast<size_t>(K_PRODUCER_COUNT) * K_ITEMS_PER_PRODUCER);

  while (remaining.load(std::memory_order_relaxed) > 0)
  {
    TestItem out{};
    if (buffer.TryPop(out))
    {
      uint64_t key = make_key(out);
      EXPECT_TRUE(seen.insert(key).second);
      if (seen_any[out.producer_id])
      {
        EXPECT_EQ(out.sequence, last_sequence[out.producer_id] + 1);
      }
      seen_any[out.producer_id] = true;
      last_sequence[out.producer_id] = out.sequence;
      remaining.fetch_sub(1, std::memory_order_relaxed);
    }
    else
    {
      std::this_thread::yield();
    }
  }

  for (auto& thread : producers)
  {
    thread.join();
  }

  EXPECT_EQ(seen.size(), K_PRODUCER_COUNT * K_ITEMS_PER_PRODUCER);
  for (uint32_t producer = 0; producer < K_PRODUCER_COUNT; ++producer)
  {
    EXPECT_TRUE(seen_any[producer]);
    EXPECT_EQ(last_sequence[producer], K_ITEMS_PER_PRODUCER - 1);
  }
}

TEST(MPSCRingBuffer, StressTest)
{
  MPSCRingBuffer<TestItem, 2048> buffer;
  std::atomic<uint32_t> remaining{K_STRESS_PRODUCERS * K_STRESS_ITEMS};
  std::vector<std::thread> producers;
  producers.reserve(K_STRESS_PRODUCERS);

  for (uint32_t producer = 0; producer < K_STRESS_PRODUCERS; ++producer)
  {
    producers.emplace_back(
        [producer, &buffer]()
        {
          for (uint32_t i = 0; i < K_STRESS_ITEMS; ++i)
          {
            TestItem item{producer, i};
            while (!buffer.TryPush(item))
            {
              std::this_thread::yield();
            }
          }
        });
  }

  std::vector<uint32_t> last_sequence(K_STRESS_PRODUCERS, 0);
  std::vector<bool> seen_any(K_STRESS_PRODUCERS, false);
  std::unordered_set<uint64_t> seen;
  seen.reserve(static_cast<size_t>(K_STRESS_PRODUCERS) * K_STRESS_ITEMS);

  while (remaining.load(std::memory_order_relaxed) > 0)
  {
    TestItem out{};
    if (buffer.TryPop(out))
    {
      uint64_t key = make_key(out);
      EXPECT_TRUE(seen.insert(key).second);
      if (seen_any[out.producer_id])
      {
        EXPECT_EQ(out.sequence, last_sequence[out.producer_id] + 1);
      }
      seen_any[out.producer_id] = true;
      last_sequence[out.producer_id] = out.sequence;
      remaining.fetch_sub(1, std::memory_order_relaxed);
    }
    else
    {
      std::this_thread::yield();
    }
  }

  for (auto& thread : producers)
  {
    thread.join();
  }

  EXPECT_EQ(seen.size(), K_STRESS_PRODUCERS * K_STRESS_ITEMS);
  for (uint32_t producer = 0; producer < K_STRESS_PRODUCERS; ++producer)
  {
    EXPECT_TRUE(seen_any[producer]);
    EXPECT_EQ(last_sequence[producer], K_STRESS_ITEMS - 1);
  }
}
