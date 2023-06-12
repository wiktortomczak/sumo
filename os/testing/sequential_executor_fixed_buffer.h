
#pragma once

#include <functional>
#include <utility>

#include "arduino-ext/pgm.h"
#include "lib/circular_buffer.h"
#include "lib/closures.h"
#include "os/executor.h"
#include "os/timer-global.h"


class SequentialExecutorFixedBuffer : public Executor {
public:
  void RunAsync(std::function<void()>&& f, const PGM<char>* description) volatile {
    tasks_.emplace_back_atomic(std::move(f), description);
  }

  void Loop() volatile {
    while (true) {
      if (!tasks_.empty()) {
        const Task& task = this_nv()->tasks_.front();
        DLOG(INFO) << P("task=\"") << (task.description || P(""))
                   << P("\"; tasks=") << tasks_.size();
        task.callable();
        tasks_.pop_front_atomic(1);
        DLOG(INFO) << P("done; tasks=") << tasks_.size();
      }
    }
  }

  void RunAfterMicros(uint32_t micros, std::function<void()>&& f) volatile {
    RunAt(timer.Now() + micros, std::move(f));
  }

  void RunEveryMicros(uint32_t period, std::function<void()>&& f) volatile {
    RunPeriodicAt(timer.Now() + period, period, std::move(f));
  }
  
  void RunAt(uint32_t micros, std::function<void()>&& f) volatile {
    RunAsync([this, micros, f = std::move(f)]() mutable {
      if (timer.Now() >= micros) {
        f();
      } else {
        RunAt(micros, std::move(f));
      }
    }, nullptr);
  }

  void RunPeriodicAt(uint32_t micros, uint32_t period,
                     std::function<void()>&& f) volatile {
    RunAsync([this, micros, period, f = std::move(f)]() mutable {
      if (timer.Now() >= micros) {
        f();
        RunPeriodicAt(micros + period, period, std::move(f));
      } else {
        RunPeriodicAt(micros, period, std::move(f));
      }
    }, nullptr);
  }


private:
  SequentialExecutorFixedBuffer* this_nv() const volatile {
    return const_cast<SequentialExecutorFixedBuffer*>(this);
  }

  struct Task {
    std::function<void()> callable;
    const PGM<char>* description;

    Task() : description(nullptr) {}
    Task(std::function<void()>&& callable_,
         const PGM<char>* description_)
      : callable(std::move(callable_)), description(description_) {}

    void swap(Task& other) {
      std::swap(callable, other.callable);
      std::swap(description, other.description);
    }
  };

  CircularBuffer<Task, 16> tasks_;  // TODO: More tasks.
};

inline volatile SequentialExecutorFixedBuffer executor_;

#define TEST_EXECUTOR executor_  // Inject executor_ into code under test.
