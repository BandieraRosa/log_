#pragma once
#include <cstddef>
#include <type_traits>

namespace br_logger
{

template <typename T, size_t Capacity>
class FixedVector
{
 public:
  bool PushBack(const T& item)
  {
    if (size_ >= Capacity) return false;
    data_[size_++] = item;
    return true;
  }

  bool PopBack()
  {
    if (size_ == 0) return false;
    --size_;
    return true;
  }

  T& operator[](size_t i) { return data_[i]; }
  const T& operator[](size_t i) const { return data_[i]; }

  size_t Size() const { return size_; }
  bool Empty() const { return size_ == 0; }
  bool Full() const { return size_ == Capacity; }

  T* begin() { return data_; }
  T* end() { return data_ + size_; }
  const T* begin() const { return data_; }
  const T* end() const { return data_ + size_; }

  void Clear() { size_ = 0; }

  static constexpr size_t GetCapacity() { return Capacity; }

 private:
  T data_[Capacity];
  size_t size_ = 0;
};

}  // namespace br_logger
