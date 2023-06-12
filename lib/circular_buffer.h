#pragma once

#include <array>
#include <iterator>
#include <utility>

#include "arduino-ext/critical_section.h"
#include "lib/check.h"


template <typename T, uint8_t capacity>
class CircularBuffer {
public:
  bool empty() const {
    return size_ == 0;
  }

  bool empty() const volatile {
    return size_ == 0;
  }

  bool full() const {
    return size_ == capacity;
  }

  uint8_t size() const { return size_; }
  uint8_t size() const volatile { return size_; }

  T& front() {
    CHECK(!empty());
    return buffer_[head_];
  }

  const T& front() const {
    CHECK(!empty());
    return buffer_[head_];
  }

  T& operator[](uint8_t i) {
    CHECK(i < size());
    return at_relative(head_, i);
  }

  const T& operator[](uint8_t i) const {
    CHECK(i < size());
    return at_relative(head_, i);
  }

  void pop_front() {
    CHECK(!empty());
    {
      T null;
      buffer_[head_].swap(null);
    }
    WrapAround(++head_);
    --size_;
  }

  void pop_front_atomic(uint8_t num) volatile {
    bool had_num_elements;
    CRITICAL_SECTION({
      had_num_elements = (num <= this_nv()->size());
      while (num--) {
        this_nv()->pop_front();
      }
    });
    CHECK(had_num_elements);
  }
  
  // void pop_front_atomic(uint8_t num) volatile {
  //   bool had_num_elements;
  //   CRITICAL_SECTION({
  //     had_num_elements = (num <= this_nv()->size());
  //     if (had_num_elements) {
  //       this_nv()->size_ -= num;
  //       while (num--) {
  //         this_nv()->buffer_[this_nv()->head_].~T();
  //         this_nv()->WrapAround(++(this_nv()->head_));
  //       }
  //     }
  //   });
  //   CHECK(had_num_elements);
  // }

  void push_back(const T& t) {
    CHECK(!full());
    at_relative(head_, size_) = t;
    ++size_;
  }

  void push_back(T&& t) {
    CHECK(!full());
    at_relative(head_, size_) = std::move(t);
    ++size_;
  }

  template <typename... Args>
  void emplace_back(Args&&... args) {
    CHECK(!full());
    new (&at_relative(head_, size_)) T(std::forward<Args>(args)...);
    ++size_;
  }

  template <typename... Args>
  uint8_t emplace_back_atomic(Args&&... args) volatile {
    uint8_t head_copy;
    uint8_t size_copy;
    CRITICAL_SECTION({
      head_copy = head_;
      size_copy = size_++;
    });

    CHECK(size_copy < capacity);
    new (&this_nv()->at_relative(head_copy, size_copy))
      T(std::forward<Args>(args)...);
    return size_copy + 1;
  }

  class iterator;
  
  iterator begin() { return iterator(this, 0); }
  iterator end() { return iterator(this, size_); }

  class iterator : public std::iterator<std::input_iterator_tag, T> {
  public:
    using reference = T&;

    iterator& operator++() { ++i_; return *this; }
    iterator operator++(int) { iterator copy = *this; ++(*this); return copy; }
    bool operator==(const iterator& other) const { return i_ == other.i_; }
    bool operator!=(const iterator& other) const { return !operator==(other); }
    reference operator*() { return (*buf_)[i_]; }
    const reference operator*() const { return (*buf_)[i_]; }

    iterator() : buf_(nullptr) {}
    iterator(const iterator& other) : buf_(other.buf_), i_(other.i_) {}
    iterator& operator=(iterator&& other) {
      this->buf_ = other.buf_;  // TODO: move_ptr().
      i_ = other.i_;
      return *this;
    }

  private:
    iterator(CircularBuffer<T, capacity>* buf, uint8_t i) : buf_(buf), i_(i) {}

    CircularBuffer<T, capacity>* buf_;
    uint8_t i_;
    friend class CircularBuffer<T, capacity>;
  };

  ~CircularBuffer() {
    while (!empty()) {
      pop_front();
    }
  }

private:
  T& at_relative(uint8_t base, uint8_t offset) {
    return buffer_[(base + offset) % capacity];
  }

  void WrapAround(uint8_t& index) const {
    if (index >= capacity) {
      index -= capacity;
    }
  }

  CircularBuffer<T, capacity>* this_nv() const volatile {
    return const_cast<CircularBuffer<T, capacity>*>(this);
  }

  std::array<T, capacity> buffer_;
  uint8_t head_ = 0;
  uint8_t size_ = 0;
};
