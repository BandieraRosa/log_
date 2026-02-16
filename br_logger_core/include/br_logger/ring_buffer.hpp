#pragma once
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <type_traits>

#include "platform.hpp"

namespace br_logger
{

template <typename T, size_t Capacity>
class MPSCRingBuffer
{
  static_assert((Capacity & (Capacity - 1)) == 0, "Capacity must be power of 2");
  static_assert(std::is_trivially_copyable_v<T>, "T must be trivially copyable");

 public:
  MPSCRingBuffer() : write_pos_(0), read_pos_(0)
  {
    for (uint32_t i = 0; i < Capacity; ++i)
    {
      buffer_[i].sequence.store(i, std::memory_order_relaxed);
    }
  }

  bool TryPush(const T& item)
  {
    uint32_t pos = write_pos_.load(std::memory_order_relaxed);
    for (;;)
    {
      Slot& slot = buffer_[pos & (Capacity - 1)];
      uint32_t seq = slot.sequence.load(std::memory_order_acquire);
      if (seq == pos)
      {
        if (write_pos_.compare_exchange_weak(pos, pos + 1, std::memory_order_relaxed,
                                             std::memory_order_relaxed))
        {
          slot.data = item;
          slot.sequence.store(pos + 1, std::memory_order_release);
          return true;
        }
        continue;
      }
      if (seq < pos)
      {
        return false;
      }
      pos = write_pos_.load(std::memory_order_relaxed);
    }
  }

  bool TryPop(T& item)
  {
    Slot& slot = buffer_[read_pos_ & (Capacity - 1)];
    uint32_t seq = slot.sequence.load(std::memory_order_acquire);
    if (seq == read_pos_ + 1)
    {
      item = slot.data;
      slot.sequence.store(read_pos_ + Capacity, std::memory_order_release);
      ++read_pos_;
      return true;
    }
    return false;
  }

  bool Empty() const
  {
    const Slot& slot = buffer_[read_pos_ & (Capacity - 1)];
    return slot.sequence.load(std::memory_order_acquire) != read_pos_ + 1;
  }

  size_t GetCapacity() const { return Capacity; }

 private:
  struct alignas(BR_LOG_CACHELINE_SIZE) Slot
  {
    std::atomic<uint32_t> sequence;
    T data;
  };

  alignas(BR_LOG_CACHELINE_SIZE) Slot buffer_[Capacity];
  alignas(BR_LOG_CACHELINE_SIZE) std::atomic<uint32_t> write_pos_;
  alignas(BR_LOG_CACHELINE_SIZE) uint32_t read_pos_;
};

}  // namespace br_logger
