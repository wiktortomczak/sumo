
#pragma once

#include <functional>
#include <memory>
#include <utility>

#include "lib/promise-impl.h"
#include "lib/template_metaprogramming.h"


// Result of a (future) computation. A value (to be) produced by some code.
// Abstracts away the difference between a result already computed and a result
// to be computed in the future, possibly asynchronously.
//
// A placeholder for a (future) value that:
//   * allows to consume the value when it is available,
//     by attaching a value handler (a callback), via Then().
//   * imposes the above as the only way of consuming the value.
//
// A Promise instance is read-only. It only allows to ultimately consume
// the value. Specifically, it does not allow to produce/set the value.
// The process producing the value is decoupled from the Promise, is left behind
// the Promise source - the function that returned it.
//
// Promises simplify asynchronous code, especially multi-step code, compared to
// callbacks:
//   * decouple generation / definition of a future result / event
//     from handling the result / event
//   * allow to capture the future result / event being produced in an object,
//     that can be passed away from result / event source and handled elsewhere
//   * lead to more terse syntax than callbacks
//
// eg.
// 
//   Promise<Event> async_event = EventThatWillHappen()  // Non-blocking.
//   // The returned Promise will be resolved when the event occurs.
//
//   async_event.Then([](auto event) { HandleEvent(event); });
//
// or equivalently:
// 
//    EventThatWillHappen().Then([](auto event) { HandleEvent(event); });
//
// but also (impossible with callbacks):
//
//    ForwardEvent(EventThatWillHappen());   // created here but handled below
//    void ForwardEvent(Promise<Event> event) { HandleEvent(event); }
//
// Partial implementation of A+ promises (https:://promisesaplus.com):
//   - without support for rejections / exceptions
//   - without support for multiple Then() handlers per instance (at most one)
//   - possibly without other features
//
// See https://developers.google.com/web/fundamentals/primers/promises
// for a complete explanation of Promise interface and for an introduction
// to programming with promises.
template <typename T>
class Promise {
public:
  using State = PromiseState<T>;  // TOOD: private

  // Registers a value handler to be called when the value is available
  // (= when the promise is resolved). Returns a child promise, resolved after
  // this promise is resolved and after the handler completes, with the
  // handler's return value.
  // The handler is run in a new call stack, sometime after the scope where
  // Then() is called ends, even if the promise is already resovled.
  template <typename ChildT>
  Promise<ChildT> Then(value_handler_t<ChildT, T>&& handler) {
    return state_->template Then<ChildT>(std::move(handler));
  }

  // Then() overload for a value handler returning a nested Promise.
  // Then() returns a child promise which 'forwards' the nested promise
  // returned by the value handler: the child promise is resolved when
  // the nested promise is resolved, with the nested promise's value.
  // The handler is run in a new call stack, sometime after the scope where
  // Then() is called ends, even if the promise is already resovled.
  template <typename ChildT>
  Promise<ChildT> Then(value_handler_t<Promise<ChildT>, T>&& handler) {
    return state_->template Then<Promise<ChildT>>(std::move(handler));
  }

  // Registers a value handler to be called when the value is available
  // (= when the promise is resolved). Unlike Then(), does not return a child
  // promise, ending the promise chain.
  // The handler is run in a new call stack, sometime after the scope where
  // ThenVoid() is called ends, even if the promise is already resovled.
  void ThenVoid(value_handler_t<void, T>&& handler) {
    state_->ThenVoid(std::move(handler));
  }

  // Creates a Promise that is already resolved with given value.
  template <typename T_ = T, typename = enable_if_not_void_t<T_>>  // SFINAE
  static Promise Resolved(const T_& value) {
    return Promise(std::make_unique<State>(value));
  }

  // Creates a Promise that is already resolved.
  template <typename T_ = T, typename = enable_if_is_void_t<T_>>  // SFINAE
  static Promise Resolved() {
    return Promise(std::make_unique<State>(true));
  }

  // TODO: Make this private. Client code should only consume the result
  // (via Then()).
  bool is_resolved() const { return state_->is_resolved(); }

public:  // TODO: protected
  Promise() : state_(new State) { }
  Promise(std::unique_ptr<State> state) : state_(state.release()) { }

  std::shared_ptr<State> state_;
};


// A Promise that can be resolved (written into).
// A write-only complementary part of a Promise instance.
// To be used at Promise source, for creating a Promise instance.
// TODO: Clean up.
template <typename T>
class PromiseWithResolve : public Promise<T> { 
  using Promise<T>::State;

public:
  PromiseWithResolve() : Promise<T>::Promise() {}

  template <typename T_ = T, typename = enable_if_not_void_t<T_>>
  void Resolve(const T_& value) {
    Promise<T>::State::Resolve(state_, value);
  }

  template <typename T_ = T, typename = enable_if_is_void_t<T_>>
  void Resolve() {
    Promise<T>::State::Resolve(state_);
  }

private:
  using Promise<T>::state_;
};
