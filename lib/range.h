
#pragma once

template <typename T>
class Range {
public:
  Range(T begin, T end) : begin_(begin), end_(end) {}

  class const_iterator;

  size_t size() const { return end_ - begin_; }

  const_iterator begin() const {
    return const_iterator(begin_);
  }

  const_iterator end() const {
    return const_iterator(end_);
  }

  class const_iterator {
  public:
    const_iterator(T value) : value_(value) {}
    const_iterator& operator++() { ++value_; return *this; }
    bool operator==(const_iterator other) const {
      return value_ == other.value_;
    }
    bool operator!=(const_iterator other) const {
      return !operator==(other);
    }
    T operator*() const { return value_; }
  private:
    T value_;
  };

private:
  const T begin_;
  const T end_;
};
