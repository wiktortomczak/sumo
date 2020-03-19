
#pragma once

#include <functional>
#include <list>
#include <utility>

#include "lib/closures.h"
#include "os/executor.h"


// Simple Executor implementation for use in tests.
// Stores callables to run in a container and runs them when Loop() is called,
// in the thread calling Loop() (typically the test's only thread).
class SequentialExecutor : public Executor {
  using Task = std::function<void()>;

public:
  template <typename F, typename... Args>
  void RunAsync(F&& callable, Args&&... args) {
    // Stores the callable for later execution in Loop().
    auto callable_args = Closures::Bind(
      std::forward<F>(callable), std::forward<Args>(args)...);
    tasks_.emplace_back(std::move(callable_args));
  }

  // Runs all callables, including possibly further callables they add,
  // until all have been run.
  void Loop() {
    while (!tasks_.empty()) {
      tasks_.front()();
      tasks_.pop_front();
    }
  }

private:
  std::list<Task> tasks_;
} executor_;

#define TEST_EXECUTOR executor_  // Inject executor_ into code under test.
