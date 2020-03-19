
#pragma once

#include <memory>
#include <vector>
#include "lib/stream.h"


// Helper utilities for Stream objects.
class Streams {
public:
  template <typename T>
  static std::unique_ptr<std::vector<T>> ToVector(Stream<T> stream) {
    auto v = std::make_unique<std::vector<T>>();
    stream.ThenEvery([v = v.get()](const T& t) { v->push_back(t); });
    return std::move(v);
  }
};
