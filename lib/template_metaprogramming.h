#pragma once

#include <functional>
#include <tuple>
#include <type_traits>
#include <utility>


template <typename U>
using enable_if_is_void_t = std::enable_if_t<std::is_void_v<U>>;

template <typename U>
using enable_if_not_void_t = std::enable_if_t<!std::is_void_v<U>>;


template <typename U>
struct fix_void {
  using type = U;
};

template <>
struct fix_void<void> {
  struct Dummy {};
  using type = Dummy;
};

template <typename U>
using fix_void_t = typename fix_void<U>::type;
