
#pragma once

#include <functional>
#include <memory>
#include <utility>

#include "lib/check.h"
#include "os/scheduler_executor-global.h"


// A sequence of asynchronously produced values / computation results.
// Values to be produced by some code.
//
// Read-only interface for consuming the values, by registering a value handler
// to be called for each value. Decoupled from the source of values.
//
// Values are not buffered. If a handler is registered after some values were
// produced, the values are lost. However,
//
// An asynchronous queue without buffering.
// Like a Promise, but for a sequence of values.
template <typename T>
class Stream {
public:
  // TODO: Move to a separate StreamWriter class.
  Stream() : state_(new State) {}

  // Registers a value handler to be called for each future value.
  // The handler is run in a new call stack, separate from the stack where
  // the value is written in the stream, sometime after it is written.
  void ThenEvery(std::function<void(const T&)>&& value_func) {
    state_->ThenEvery(std::move(value_func));
  }

  // TODO: Move to a separate StreamWriter class.
  void Put(const T& value) {
    state_->Put(value);
  }

  // TODO: Move to a separate StreamWriter class.
  void PutAsync(const T& value) {
    state_->Put(value);
  }

private:
  class State {
  public:
    void Put(const T& value) {
      executor.RunAsync(value_func_, value);
    }

    void PutAsync(const T& value) {
      executor.RunAsync([this, value]() { value_func_(value); });
    }

    void ThenEvery(std::function<void(const T&)>&& value_func) {
      CHECK(!value_func_);
      value_func_ = std::move(value_func);
    }
    
    std::function<void(const T&)> value_func_;
  };

  std::shared_ptr<State> state_;
};
