
#pragma once

#include <tuple>
#include <utility>


class Closures {
public:
  // TODO: Replace with std::bind?
  template <typename F, typename... Args>
  static auto Bind(F&&f, Args&&... args) {
    return 
      [f = std::forward<F>(f),
       args = std::make_tuple(std::forward<Args>(args)...)]() mutable {
        std::apply(std::move(f), std::move(args));
      };
  }
};
