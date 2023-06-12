
#include <tuple>

#include "lib/tuples.h"


template <auto V>
struct ValueType {
  static constexpr decltype(V) value = V;
};


template <auto... values>
struct value_list {
private:
  using tuple_ = std::tuple<ValueType<values>...>;

public:
  static constexpr size_t size = tuple_size_v<tuple_>;

  template <typename F>
  static void ForEach(F&& f) {
    Tuples::ForEachType
  //   auto f_var_args = [f = std::move(f)](const Args&... args) {
  //     (f.template operator()<Args>(args), ...);
  //   };
  //   std::apply(std::move(f_var_args), t);
  // }

};
