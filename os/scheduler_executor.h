
#pragma once

#include "lib/closures.h"
#include "os/executor.h"
#include "os/scheduler.h"


// Executor implementation that delegates to a global Scheduler.
// Note: Scheduler.Loop() must be called for it to finally run the callables.
class SchedulerExecutor : public Executor {
public:
  template <typename F, typename... Args>
  void RunAsync(F&& callable, Args&&... args) volatile {
    auto callable_args = Closures::Bind(
      std::forward<F>(callable), std::forward<Args>(args)...);
    scheduler.RunAfterMicros(0, std::move(callable_args), P("RunAsync()"));
  }
};
