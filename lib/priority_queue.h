#pragma once

#include <algorithm>
#include <queue>
#include <vector>
#include "lib/check.h"


template <typename T,
          typename ContainerT = std::vector<T>,
          typename CompareT = std::less<typename ContainerT::value_type>>
class PriorityQueue : public std::priority_queue<T, ContainerT, CompareT> {
  using priority_queue = std::priority_queue<T, ContainerT, CompareT>;
  using priority_queue::c;
  using priority_queue::comp;
  
public:
  template <typename IteratorT>
  void move(IteratorT tasks_begin, IteratorT tasks_end) {
    for (auto it = tasks_begin; it != tasks_end; ++it) {
      c.emplace_back(std::move(*it));
    }
    Order();
  }

  // https://stackoverflow.com/questions/19467485
  template <typename F>
  void RemoveSingle(F&& predicate) {
    auto it = std::find_if(c.begin(), c.end(), predicate);
    CHECK(it != c.end());
    if (&(*it) != &c.back()) {
      *it = std::move(c.back());
      c.pop_back();
      Order();
    } else {
      c.pop_back();
      // TODO: Need to Order()?
    }
  }

  void Order() {
    std::make_heap(c.begin(), c.end(), comp);
  }
};
