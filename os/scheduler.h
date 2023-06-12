// #error "why included?"

#pragma once

#include <functional>
#include <optional>
#include <utility>

#include "arduino-ext/critical_section.h"
#include "lib/check.h"
#include "lib/circular_buffer.h"
#include "lib/fixed_capacity_vector.h"
#include "lib/log.h"
#include "lib/priority_queue.h"
#include "lib/template_metaprogramming.h"
#include "os/timer-global.h"
#include "os/thread.h"

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
// TODO: thread safety.
//
// TODO: Use C++ duration<> in place of uint32_t to disambiguate time units.
template <typename DescriptionT = const char>
class Scheduler {
protected:
  static constexpr size_t MAX_TASKS = 24;
  static constexpr size_t MAX_NEW_TASKS = 8;  // TODO: Merge into single queue.

  struct Task;
  struct TaskGreater;
  using TaskQueue = PriorityQueue<
    Task, FixedCapacityVector<Task, MAX_TASKS>, TaskGreater>;
  using NewTaskQueue = CircularBuffer<Task, MAX_TASKS>;

public:
  using TaskId = uint8_t;

  // Schedules a callable to be run after given number of microseconds.
  TaskId RunAfterMicros(uint32_t micros, std::function<void()>&& callable,
                        DescriptionT* description = nullptr) volatile {
    return EmplaceNewTask(
      timer.Now() + micros, 0, std::move(callable), description);
  }

  // Schedules a callable to be run repeatedly every given number
  // of microseconds, starting after one such period.
  TaskId RunEveryMicros(uint32_t micros, std::function<void()>&& callable,
                        DescriptionT* description = nullptr) volatile {
    return EmplaceNewTask(
      timer.Now() + micros, micros, std::move(callable), description);
  }

  // Schedules a callable to be run repeatedly every given number
  // of microseconds, starting after one such period, until the callable
  // returns true.
  TaskId RunEveryMicrosUntil(uint32_t micros, std::function<bool()>&& callable,
                             DescriptionT* description = nullptr) volatile {
    // This is not thread safe! TODO: Use task_id returned from RunEveryMicros.
    return RunEveryMicros(
      micros, [this, callable = std::move(callable), task_id = next_task_id_]() {
        if (callable()) {
          Cancel(task_id);
        }
      }, description);
  }

  // Cancels a scheduled callable.
  void Cancel(TaskId task_id) volatile {
    // TODO: thread-safe.
    this_nv()->tasks_.RemoveSingle(
      [task_id](const Task& task) { return task.id == task_id; });
    DLOG(INFO) << "tasks=" << this_nv()->tasks_.size();
  }

  // Runs scheduled callables, including possibly further callables they add,
  // until all have been run. A periodic callable persists - keeps the loop
  // running - unless it is canceled.
  // Typically, this is the application's main event loop and does not return.
  // See class doc for more information.
  void Loop() volatile {
    while (!this_nv()->tasks_.empty() || !this_nv()->new_tasks_.empty()) {
      if (!this_nv()->new_tasks_.empty()) {
        MergeNewTasksIntoTasks();
      }
      // TODO: Clean up.
      const_cast<Task&>(this_nv()->tasks_.top())
        .RunIfTimeAndUpdateTasks(&(this_nv()->tasks_));
    }
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
  Promise<void> AfterMicros(uint32_t micros) volatile;

  // Schedules a callable to be run repeatedly every given number
  // of microseconds, starting after one such period, until the callable
  // returns a defined value. Resolves the returned promise with the value.
  //
  // Similar to RunEveryMicrosUntil() but allows to chain dependent actions
  // to the Promise.
  // TODO: Promise::Resolver
  template <typename T, typename = enable_if_not_void_t<T>>  // SFINAE
  Promise<T> RunEveryMicrosUntilResolved(
    uint32_t micros, std::function<std::optional<fix_void_t<T>>()>&& callable,
    DescriptionT* description = nullptr) volatile {
    PromiseWithResolve<T> promise;
    RunEveryMicrosUntil(micros, [callable = std::move(callable), promise]() {
      const std::optional<T> result = callable();
      if (result) {
        promise.Resolve(result);
        return true;
      } else {
        return false;
      }
    }, description);
    return promise;
  }

  // Overload of RunEveryMicrosUntilResolved() for bool-returning callables.
  // The returned promise is resolved when the callable returns true.
  template <typename T = void, typename = enable_if_is_void_t<T>>  // SFINAE
  Promise<T> RunEveryMicrosUntilResolved(
    uint32_t micros, std::function<bool()>&& callable,
    DescriptionT* description = nullptr) volatile {
    PromiseWithResolve<T> promise;
    RunEveryMicrosUntil(micros, [callable = std::move(callable), promise]() mutable {
      if (callable()) {
        promise.Resolve();
        return true;
      } else {
        return false;
      }
    }, description);
    return promise;
  }

protected:
  TaskId EmplaceNewTask(uint32_t time, uint32_t period,
                        std::function<void()>&& callable,
                        DescriptionT* description) volatile {
    TaskId task_id;
    size_t num_tasks;
    CRITICAL_SECTION({
      // TODO: Separate task_ids for periodic tasks TODO.
      task_id = next_task_id_++;
      num_tasks = this_nv()->tasks_.size();
    });
    const size_t num_new_tasks = new_tasks_.emplace_back_atomic(
      task_id, time, period, std::move(callable), description);
    if (!Thread::is_interrupt()) {
      DLOG(INFO) << P("tasks=") << num_tasks
                 << P(" new_tasks=") << num_new_tasks;
    }
    return task_id;
  }

  void MergeNewTasksIntoTasks() volatile {
    typename NewTaskQueue::iterator new_tasks_begin;
    typename NewTaskQueue::iterator new_tasks_end;
    size_t num_tasks;
    CRITICAL_SECTION({
      new_tasks_begin = this_nv()->new_tasks_.begin();
      new_tasks_end = this_nv()->new_tasks_.end();
      num_tasks = this_nv()->tasks_.size();
    });

    this_nv()->tasks_.move(new_tasks_begin, new_tasks_end);
    const auto num_tasks_merged =
      std::distance(new_tasks_begin, new_tasks_end);
    this_nv()->new_tasks_.pop_front_atomic(num_tasks_merged);
    DLOG(INFO) << P("tasks=") << num_tasks + num_tasks_merged;
  }

  struct Task {
    Task(TaskId id_, uint32_t time_, uint32_t period_,
         std::function<void()>&& callable_, DescriptionT* description_)
      : time(time_), period(period_), description(description_),
        callable(std::move(callable_)), id(id_) { }
    Task() {} // Needed by FixedCapacityVector.

    void RunIfTimeAndUpdateTasks(TaskQueue* tasks) {
      if (time <= timer.Now()) {
        if (!period) {
          CHECK(this == &tasks->top());
          std::function<void()> callable_(std::move(callable));
          tasks->pop();
          DLOG(INFO) << P("other tasks=") << tasks->size();
          LogCall();
          callable_();
        } else {
          time += period;  // This temporarily breaks the heap property.
          // TODO: callable() could possibly add more tasks. 
          // Is queue.push() / emplace() ok while the heap property is broken?
          LogCall();
          callable();
          tasks->Order();
        }
      }
    }

  private:
    void LogCall() {
      if (description) {
        DLOG(INFO) << P("task=") << id << P(" description=") << description;
      } else {
        DLOG(INFO) << P("task=") << id;
      }
    }
    
  public:  // TODO: private.
    uint32_t time;
    // TODO: All remainining members const?
    uint32_t period;
    DescriptionT* description;
    std::function<void()> callable;
    TaskId id;
  } __attribute__((packed));

  struct TaskGreater {
    constexpr bool operator()(const Task& a, const Task& b) const {
      return a.time > b.time;
    }
  };

  // Non-volatile. Members accessed via this_nv
  // are sure to be accessed by this thread only.
  Scheduler* this_nv() const volatile { return const_cast<Scheduler*>(this); }

  TaskQueue tasks_;
  NewTaskQueue new_tasks_;
  uint32_t next_task_id_ = 0;
};
