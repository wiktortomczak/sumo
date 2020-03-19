
#pragma once

#include <functional>
#include <optional>
#include <utility>
#include <vector>

#include "lib/check.h"
#include "lib/priority_queue.h"
#include "lib/template_metaprogramming.h"
#include "os/timer-global.h"


template <typename T> class Promise;
template <typename T> class PromiseWithResolve;


// Runs given pieces of code at given time in the future. Allows to schedule
// their future execution.
//
// Provides for cooperative multitasking of multiple pieces of scheduled code
// (see below).
//
// Monitors the global Timer and runs scheduled code when its time is due.
//
// Does not come with its own thread of execution. Instead, requires that Loop()
// is called for it to run scheduled code (in the Loop() caller's thread).
//
// Execution of all scheduled code is sequential (single-threaded). A subsequent
// piece of scheduled code is run only after the piece of code being run
// completes. If a piece of code takes too long to execute, the execution of
// subsequent pieces of scheduled code is delayed. For this reason, all
// scheduled code should complete quickly, per the cooperative multitasking
// requirement. To this end, a long-running piece of code should defer a part of
// its execution for later, by calling the scheduler back, and return - as though
// it yielded in a scheduler with coroutines.
//
// Callables being run may call back the scheduler and schedule further
// callables. Note: At least one callable must be scheduled before calling
// Loop(), otherwise it returns immediately.
//
// TODO: Use C++ duration<> in place of uint32_t to disambiguate time units.
template <bool has_stop = false>
class Scheduler {
protected:
  struct Task;
  struct TaskGreater;
  // TODO: Replace vector with a fixed-size CircularBuffer.
  using TaskQueue = PriorityQueue<Task, std::vector<Task>, TaskGreater>;

public:
  using TaskId = int16_t;

  // Schedules a callable to be run after given number of microseconds.
  TaskId RunAfterMicros(uint32_t micros, std::function<void()>&& callable) {
    const uint32_t time = timer.Now() + micros;
    const TaskId task_id = next_task_id_++;
    tasks_.emplace(task_id, time, 0, std::move(callable));
    return task_id;
  }

  // Schedules a callable to be run repeatedly every given number
  // of microseconds, starting after one such period.
  TaskId RunEveryMicros(uint32_t micros, std::function<void()>&& callable) {
    const uint32_t time = timer.Now() + micros;
    const TaskId task_id = next_task_id_++;
    tasks_.emplace(task_id, time, micros, std::move(callable));
    return task_id;
  }

  // Schedules a callable to be run repeatedly every given number
  // of microseconds, starting after one such period, until the callable
  // returns true.
  TaskId RunEveryMicrosUntil(uint32_t micros, std::function<bool()>&& callable) {
    return RunEveryMicros(
      micros, [this, callable = std::move(callable), task_id = next_task_id_]() {
        if (callable()) {
          Cancel(task_id);
        }
      });
  }

  // Cancels a scheduled callable.
  void Cancel(TaskId task_id) {
    tasks_.RemoveSingle(
      [task_id](const Task& task) { return task.id == task_id; });
  }

  // Runs scheduled callables, including possibly further callables they add,
  // until all have been run. A periodic callable persists - keeps the loop
  // running - unless it is canceled.
  // Typically, this is the application's main event loop and does not return.
  // See class doc for more information.
  void Loop() {
    while (!tasks_.empty()) {
      const_cast<Task&>(tasks_.top()).RunIfTimeAndUpdateTasks(&tasks_);

      // For use in tests.
      if constexpr (has_stop) {
        if (should_stop_) {
          break;
        }
      }
    }
  }

  // For use in tests. TODO: Define only if has_stop.
  void Stop() {
    should_stop_ = true;
  }

  // Returns a promise resolved after given number of microseconds.
  //
  // Similar to RunAfterMicros(), but decouples waiting for micros to pass
  // from the action to take after and allows to construct the action chain
  // using Promises.
  // eg. instead of scheduler.RunAfterMicros(micros, f)
  //     allows     scheduler.AfterMicros(micros).Then(f).Then(...
  //
  // #include "scheduler-promise.h" for definition, after "lib/promise.h"
  // has been included to define Promise<T>. This is to break circular
  // dependency Scheduler -> Promise -> SchedulerExecutor -> Scheduler.
  Promise<void> AfterMicros(uint32_t micros);

  // Schedules a callable to be run repeatedly every given number
  // of microseconds, starting after one such period, until the callable
  // returns a defined value. Resolves the returned promise with the value.
  //
  // Similar to RunEveryMicrosUntil() but allows to chain dependent actions
  // to the Promise.
  // TODO: Promise::Resolver
  template <typename T, typename = enable_if_not_void_t<T>>  // SFINAE
  Promise<T> RunEveryMicrosUntilResolved(
    uint32_t micros, std::function<std::optional<fix_void_t<T>>()>&& callable) {
    PromiseWithResolve<T> promise;
    RunEveryMicrosUntil(micros, [callable = std::move(callable), promise]() {
      const std::optional<T> result = callable();
      if (result) {
        promise.Resolve(result);
        return true;
      } else {
        return false;
      }
    });
    return promise;
  }

  // Overload of RunEveryMicrosUntilResolved() for bool-returning callables.
  // The returned promise is resolved when the callable returns true.
  template <typename T = void, typename = enable_if_is_void_t<T>>  // SFINAE
  Promise<T> RunEveryMicrosUntilResolved(
    uint32_t micros, std::function<bool()>&& callable) {
    PromiseWithResolve<T> promise;
    RunEveryMicrosUntil(micros, [callable = std::move(callable), promise]() mutable {
      if (callable()) {
        promise.Resolve();
        return true;
      } else {
        return false;
      }
    });
    return promise;
  }

protected:
  struct Task {
    Task(TaskId id_, uint32_t time_, uint32_t period_,
         std::function<void()>&& callable_)
      : id(id_), time(time_), period(period_),
        callable(std::move(callable_)) { }
    
    void RunIfTimeAndUpdateTasks(TaskQueue* tasks) {
      if (time <= timer.Now()) {
        if (!period) {
          CHECK(this == &tasks->top());
          std::function<void()> callable_(std::move(callable));
          tasks->pop();
          callable_();
        } else {
          time += period;  // This temporarily breaks the heap property.
          // TODO: callable() could possibly add more tasks. 
          // Is queue.push() / emplace() ok while the heap property is broken?
          callable();
          tasks->Order();
        }
      }
    }

    TaskId id;
    uint32_t time;
    uint32_t period;
    std::function<void()> callable;
  };

  struct TaskGreater {
    constexpr bool operator()(const Task& a, const Task& b) const {
      return a.time > b.time;
    }
  };

  TaskQueue tasks_;
  uint32_t next_task_id_ = 0;
  bool should_stop_ = false;  // TODO: Define only if has_stop.
};
