
#pragma once

#include "os/scheduler.h"

// Scheduler variant that can be stopped after given time.
class TestScheduler : public Scheduler {
public:
  // Runs at most for given duration.
  void Loop(uint32_t duration_usec) {
    Loop();
    RunAfterMicros(duration_usec, [this]() { Stop(); });
  }

  void Loop() {
    while (!tasks_.empty() && !should_stop_) {
      const_cast<Task&>(tasks_.top()).RunIfTimeAndUpdateTasks(&tasks_);
    }
  }

  void Stop() {
    should_stop_ = true;
  }

private:
  bool should_stop_ = false;
} scheduler_;

#define TEST_SCHEDULER scheduler_  // Inject scheduler_ into code under test.
