#pragma once

#include <algorithm>
#include <queue>
#include <vector>
#include "lib/check.h"

namespace internal {
  template <typename IteratorT>
  IteratorT next(IteratorT it) { ++it; return it; }
}  // internal

template <typename T,
          typename ContainerT = std::vector<T>,
          typename CompareT = std::less<typename ContainerT::value_type>>
class PriorityQueue : public std::priority_queue<T, ContainerT, CompareT> {
  // using std::priority_queue<T, ContainerT, CompareT>::c;
  using std::priority_queue<T, ContainerT, CompareT>::comp;
  
public:
  using std::priority_queue<T, ContainerT, CompareT>::c;

  // https://stackoverflow.com/questions/19467485
  template <typename F>
  void RemoveSingle(F&& predicate) {
    auto it = std::find_if(c.begin(), c.end(), predicate);
    CHECK(it != c.end());
    if (internal::next(it) != c.end()) {
      *it = c.back();
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
