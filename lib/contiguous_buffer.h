
#include <cstdint>
#include <cstring>
#include <type_traits>

#include "lib/check.h"


template <size_t buffer_size>
class ContiguousBuffer {
public:
  template <typename T, typename = std::enable_if_t<!std::is_pointer_v<T>>>
  void push_back(const T& t) {
    CHECK(free_capacity() >= sizeof(t));
    std::memcpy(tail_, &t, sizeof(t));
    tail_ += sizeof(t);
  }

  // TODO: Remove?
  template <typename T, typename = std::enable_if_t<std::is_pointer_v<T>>>
  void push_back(const T t) {
    CHECK(free_capacity() >= sizeof(t));
    *(reinterpret_cast<T*>(tail_)) = t;
    tail_ += sizeof(t);
  }

  bool empty() const {
    return head_ == tail_;
  }

  size_t size() const {
    return tail_ - head_;
  }

  size_t free_capacity() const {
    return data_ + buffer_size  - tail_;
  }

  template <typename T = uint8_t>
  const T& front() const {
    return peek<T>(0);
  }

  template <typename T = uint8_t>
  const T& peek(size_t offset) const {
    CHECK(size() >= offset + sizeof(T));
    return reinterpret_cast<T*>(head_ + offset)[0];
  }

  template <typename T = uint8_t>
  T& front() {
    return reinterpret_cast<T*>(head_)[0];
  }

  // TODO: Broken! Call destructor.
  void pop_front(size_t size) {
    head_ += size;
  }

  void Reset() {
    head_ = tail_ = data_;
  }

private:
  uint8_t data_[buffer_size];
  uint8_t* head_ = data_;
  uint8_t* tail_ = data_;
};
