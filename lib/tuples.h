
#pragma once

#include <tuple>
#include <type_traits>
#include <utility>


class Tuples {
public:
  template <typename F, typename... Args>
  static void ForEach(const std::tuple<Args...>& t, F&& f) {
    auto f_var_args = [f = std::move(f)](const Args&... args) {
      (f.template operator()<Args>(args), ...);
    };
    std::apply(std::move(f_var_args), t);
  }
  
  // template <typename F, typename... Args>
  // // TODO: Explicit return type std::tuple<decltype(F(Args))...>.
  // static auto Map(const std::tuple<Args...>& t, F&& f) {
  //   auto f_var_args = [f = std::move(f)](const Args&... args) {
  //     return make_tuple((f.template operator()<Args>(args), ...));
  //   };
  //   return std::apply(std::move(f_var_args), t);
  // }

private:
  template <typename TupleT>
  struct ForEachTypeImpl;

  template <typename... Args>
  struct ForEachTypeImpl<std::tuple<Args...>> {
    template <typename F>
    void operator()(F&& f) const {
      ((f.template operator()<Args>()), ...);
    }
  };

public:
  template <typename TupleT, typename F>
  static void ForEachType(F&& f) {
    ForEachTypeImpl<TupleT>()(std::move(f));
  }
};


template <typename... Ts>
class TupleBuilder {
  template <typename T, bool decay>
  using maybe_decay_t = std::conditional_t<decay, std::decay_t<T>, T>;

public:
  TupleBuilder() { static_assert(sizeof...(Ts) == 0); }
  TupleBuilder(std::tuple<Ts...>&& tuple) : tuple_(std::move(tuple)) {}

  template <typename T, bool decay = false>
  TupleBuilder<Ts..., maybe_decay_t<T, decay>> Add(T&& t) && {
    using mdT = maybe_decay_t<T, decay>;
    return TupleBuilder<Ts..., mdT>(
      std::tuple_cat(move(tuple_), std::tuple<mdT>(std::forward<T>(t))));
  }

  std::tuple<Ts...> Build() { return tuple_; }

private:
  std::tuple<Ts...> tuple_;
};
