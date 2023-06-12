
#pragma once

#include <array>
#include <utility>

#include "lib/check.h"
#include "lib/log.h"


template <typename T, size_t capacity>
class FixedCapacityVector {
  using Array = std::array<T, capacity>;

public:
  using value_type = T;
  using reference = T&;
  using const_reference = const T&;
  using size_type = std::size_t;

  using iterator = typename Array::iterator;
  using const_iterator = typename Array::const_iterator;
  
  bool empty() const { return size_ == 0; }
  bool full() const { return size_ == capacity; }
  size_t size() const { return size_; }
  T& front() { CHECK(!empty()); return data_[0]; }
  const T& front() const { CHECK(!empty()); return data_[0]; }
  T& back() { CHECK(!empty()); return data_[size_ - 1]; }
  const T& back() const { CHECK(!empty()); return data_[size_ - 1]; }

  void push_back(const T& t) {
    CHECK(!full());
    data_[size_++] = t;
  }

  void push_back(T&& t) {
    CHECK(!full());
    data_[size_++] = std::move(t);
  }

  template <typename... Args>
  void emplace_back(Args&&... args) {
    CHECK(!full());
    new (&data_[size_++]) T(std::forward<Args>(args)...);
  }

  void pop_back() {
    CHECK(!empty());
    --size_;
  }

  template<typename InputIteratorT>
  iterator insert(const_iterator position,
                  InputIteratorT first, InputIteratorT last) {
    LOG(FATAL) << P("Not implemented");
  }

  iterator begin() { return data_.data(); }
  const_iterator begin() const { return data_.data(); }
  iterator end() { return data_.data() + size_; }
  const_iterator end() const { return data_.data() + size_; }

private:
  Array data_;
  size_t size_ = 0;
};
