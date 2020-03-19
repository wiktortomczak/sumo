
#pragma once

#include "os/scheduler.h"

// Scheduler variant that can be stopped after given time.
class TestScheduler : public Scheduler<true> {
public:
  // Runs at most for given duration.
  void Loop(uint32_t duration_usec) {
    RunAfterMicros(duration_usec, [this]() { Stop(); });
    Scheduler<true>::Loop();
  }
} scheduler_;

#define TEST_SCHEDULER scheduler_  // Inject scheduler_ into code under test.
