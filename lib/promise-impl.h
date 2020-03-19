// Private implementation of lib/promise.h. Should only be included from there.

#pragma once

#include <functional>
#include <memory>
#include <type_traits>
#include <utility>

#include "lib/check.h"
#include "lib/template_metaprogramming.h"
#include "os/scheduler_executor-global.h"

template <typename T> class Promise;


template <typename T>
struct promise_t_impl {
  using type = Promise<T>;
};

template <typename T>
struct promise_t_impl<Promise<T>> {
  using type = Promise<T>;
};

template <typename T>
using promise_t = typename promise_t_impl<T>::type;


template <typename ResultT, typename ArgT>
struct value_handler {
  using type = std::function<ResultT(const ArgT&)>;
};

template <typename ResultT>
struct value_handler<ResultT, void> {
  using type = std::function<ResultT(void)>;
};

template <typename ResultT, typename ArgT>
using value_handler_t = typename value_handler<ResultT, ArgT>::type;


  
template <typename T>
class PromiseState {
  using State = PromiseState<T>;

public:
  PromiseState() { }

  template <typename T_ = T, typename = enable_if_not_void_t<T_>>
  PromiseState(const T_& value) : value_(value) { }

  template <typename ChildTOrPromiseChildT,
            typename PromiseChildT = promise_t<ChildTOrPromiseChildT>>
  PromiseChildT Then(value_handler_t<ChildTOrPromiseChildT, T>&& f) {
    CHECK(!on_resolved_);
    PromiseChildT child;
    // TODO: bind.
    if constexpr (!std::is_void_v<T>) {
      on_resolved_ =
        [f_ = std::move(f), child_state = child.state_](const T& value) mutable {
          CallFuncAndResolveChild<ChildTOrPromiseChildT>(
            std::move(f_), value, std::move(child_state));
        };
    } else {
      on_resolved_ =
        [f_ = std::move(f), child_state = child.state_]() mutable {
          CallFuncAndResolveChild<ChildTOrPromiseChildT>(
            std::move(f_), std::move(child_state));
        };
    }
    if (is_resolved()) {
      if constexpr (!std::is_void_v<T>) {
        executor.RunAsync(on_resolved_, value_.value());
      } else {
        executor.RunAsync(on_resolved_);
      }
    }
    return child;
  }
  
  void ThenVoid(value_handler_t<void, T>&& f) {
    CHECK(!on_resolved_);
    if (!is_resolved()) {
      on_resolved_ = std::move(f);
    } else {  // is_resolved
      if constexpr (!std::is_void_v<T>) {
        executor.RunAsync(std::move(f), value_.value());
      } else {
        executor.RunAsync(std::move(f));
      }
    }
  }
  
  template <typename T_ = T, typename = enable_if_not_void_t<T_>>
  static void Resolve(std::shared_ptr<State> this_ptr, const T_& value) {
    State* const this_ = this_ptr.get();
    CHECK(!this_->is_resolved());
    this_->value_ = value;
    if (this_->on_resolved_) {
      executor.RunAsync(this_->on_resolved_, value);
    }
  }

  template <typename T_ = T, typename = enable_if_is_void_t<T_>>
  static void Resolve(std::shared_ptr<State> this_ptr) {
    State* const this_ = this_ptr.get();
    CHECK(!this_->is_resolved());
    this_->value_ = true;
    if (this_->on_resolved_) {
      executor.RunAsync(this_->on_resolved_);
    }
  }

  template <typename T_ = T, typename = enable_if_not_void_t<T_>>
  static void Resolve(std::shared_ptr<State> this_ptr,
                      Promise<T_>&& value_promise) {
    value_promise.ThenVoid(
      [this_ptr](const T& value) { Resolve(this_ptr, value); });
  }

  template <typename T_ = T, typename = enable_if_is_void_t<T_>>
  static void Resolve(std::shared_ptr<State> this_ptr,
                      Promise<void>&& value_promise) {
    value_promise.ThenVoid([this_ptr]() { Resolve(this_ptr); });
  }

public:
  bool is_resolved() const { return value_.has_value(); }

private:
  template <typename ChildTOrPromiseChildT,
            typename ChildPromiseT = promise_t<ChildTOrPromiseChildT>,
            typename T_ = T, typename = enable_if_not_void_t<T_>>
  static void CallFuncAndResolveChild(
    std::function<ChildTOrPromiseChildT(const T_&)>&& f, const T_& value,
    std::shared_ptr<typename ChildPromiseT::State> child_state) {
    if constexpr (!std::is_void_v<ChildTOrPromiseChildT>) {
      ChildPromiseT::State::Resolve(std::move(child_state), f(value));
    } else {
      f(value);
      ChildPromiseT::State::Resolve(std::move(child_state));
    }
  }

  template <typename ChildTOrPromiseChildT,
            typename ChildPromiseT = promise_t<ChildTOrPromiseChildT>,
            typename T_ = T, typename = enable_if_is_void_t<T_>>
  static void CallFuncAndResolveChild(
    std::function<ChildTOrPromiseChildT()>&& f,
    std::shared_ptr<typename ChildPromiseT::State> child_state) {
    if constexpr (!std::is_void_v<ChildTOrPromiseChildT>) {
      ChildPromiseT::State::Resolve(std::move(child_state), f());
    } else {
      f();
      ChildPromiseT::State::Resolve(std::move(child_state));
    }
  }

  value_handler_t<void, T> on_resolved_;
  std::optional<std::conditional_t<!std::is_void_v<T>, T, bool>> value_;
};
