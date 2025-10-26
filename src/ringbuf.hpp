#pragma once
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <type_traits>

namespace nhft {

template <typename T>
class SpscRing {
  static_assert(std::is_trivially_copyable<T>::value, "T must be trivially copyable");
public:
  explicit SpscRing(size_t capacity_pow2)
      : capacity_(capacity_pow2), mask_(capacity_pow2 - 1), max_depth_(0) {
    // capacity must be power of two
    if ((capacity_pow2 & (capacity_pow2 - 1)) != 0) {
      capacity_ = 1024;
      mask_ = capacity_ - 1;
    }
    buf_ = new T[capacity_];
    head_.store(0, std::memory_order_relaxed);
    tail_.store(0, std::memory_order_relaxed);
  }
  ~SpscRing() { delete[] buf_; }

  bool push(const T& v) {
    auto head = head_.load(std::memory_order_relaxed);
    auto next = head + 1;
    auto tail = tail_.load(std::memory_order_acquire);
    if (next - tail > capacity_) {
      // full
      return false;
    }
    buf_[head & mask_] = v;
    head_.store(next, std::memory_order_release);
    return true;
  }

  bool pop(T& out) {
    auto tail = tail_.load(std::memory_order_relaxed);
    auto head = head_.load(std::memory_order_acquire);
    if (tail == head) return false;
    out = buf_[tail & mask_];
    tail_.store(tail + 1, std::memory_order_release);
    // track depth on consumer side
    auto depth = head - (tail + 1);
    if (depth > (int64_t)max_depth_) max_depth_ = (size_t)depth;
    return true;
  }

  size_t depth() const {
    auto head = head_.load(std::memory_order_acquire);
    auto tail = tail_.load(std::memory_order_acquire);
    return (size_t)(head - tail);
  }

  size_t capacity() const { return capacity_; }
  size_t max_depth() const { return max_depth_; }

private:
  T* buf_{};
  size_t capacity_{};
  size_t mask_{};
  alignas(64) std::atomic<uint64_t> head_{0};
  alignas(64) std::atomic<uint64_t> tail_{0};
  alignas(64) std::atomic<size_t> padding_{0}; // to keep indices on separate cache lines
  size_t max_depth_;
};

} // namespace nhft
