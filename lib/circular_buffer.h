#pragma once

#include <array>
#include <utility>
#include "lib/check.h"


template <typename T, size_t capacity>
class CircularBuffer {
public:
  bool empty() const {
    return size_ == 0;
  }

  bool full() const {
    return size_ == capacity;
  }

  size_t size() const { return size_; }

  const T& front() const {
    CHECK(!empty());
    return buffer_[head_];
  }

  void pop_front() {
    CHECK(!empty());
    if (++head_ == capacity) {
      head_ = 0;
    }
    --size_;
  }

  void push_back(T&& t) {
    CHECK(!full());
    buffer_[(head_ + size_) % capacity] = t;
    ++size_;
  }

  void emplace_back(T&& f) {
    CHECK(!full());
    new(&buffer_[(head_ + size_) % capacity]) T(std::move(f));
    ++size_;
  }

  // TODO
  // template <typename... Args>
  // void emplace_back(Args&&... args) {
  //   printf("emplace_back() head=%zu size=%zu\n", head_, size_);
  //   CHECK(!full());
  //   new(&buffer_[(head_ + size_) % capacity]) T(std::forward<Args>(args)...);
  //   ++size_;
  // }

  std::array<T, capacity> buffer_;
  size_t head_ = 0;
  size_t size_ = 0;
};
