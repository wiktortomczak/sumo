
#include <cstdint>


#define TEST_CRITICAL_SECTION(code) code  // TODO


class FakeTimer {
public:
  uint32_t Now() {
    return now_++;
  };

  void Reset() { now_ = 0; }

private:                                 
  uint32_t now_ = 0;
} timer_;

#define TEST_TIMER timer_  // Inject Fake timer into code under test.

#include "os/scheduler.h"

using TaskId = Scheduler<const char>::TaskId;

class SchedulerProxy {
public:
  TaskId RunAfterMicros(uint32_t micros, std::function<void()>&& f,
                        const char* description = nullptr) volatile {
    return scheduler_->RunAfterMicros(micros, std::move(f), description);
  }

  void Set(volatile Scheduler<const char>* scheduler) {
    scheduler_ = scheduler;
  }

private:
  volatile Scheduler<const char>* scheduler_ = nullptr;
} scheduler_;

#define TEST_SCHEDULER scheduler_  // Inject scheduler_ into code under test.
#include "lib/promise.h"
#include "os/scheduler-promise.h"


class Call {
public:
  operator bool() const { return time_.has_value(); }
  uint32_t time() const { return time_.value(); }

  void Make() {
    time_ = timer_.Now();
  }

private:
  std::optional<uint32_t> time_;
};

  
void AssertInRange(uint32_t t, uint32_t a, uint32_t b) {
  assert(a <= t && t < b);
}


int main() {
  {
    volatile Scheduler scheduler;
    scheduler_.Set(&scheduler);
    timer_.Reset();

    Call calls[5];
    for (int i : {2, 1, 4, 3}) {
      scheduler.RunAfterMicros(i*100, [&calls, i]() { calls[i].Make(); });
    }
    scheduler.Loop();

    AssertInRange(calls[1].time(), 100, 105);
    AssertInRange(calls[2].time(), 200, 205);
    AssertInRange(calls[3].time(), 300, 305);
    AssertInRange(calls[4].time(), 400, 405);
  }

  // TODO: Test scheduling a new task from inside a task, while scheduler is running.

  {
    volatile Scheduler scheduler;
    scheduler_.Set(&scheduler);
    timer_.Reset();

    Call calls[5];
    Call* call = &calls[1];   
    auto scheduled = scheduler.RunEveryMicros(
      100, [&call]() { call->Make(); ++call; });
    scheduler.RunAfterMicros(250, [&]() { scheduler.Cancel(scheduled); });
    scheduler.Loop();

    AssertInRange(calls[1].time(), 100, 105);
    AssertInRange(calls[2].time(), 200, 205);
    assert(!calls[3]);
  }

  {
    volatile Scheduler scheduler;
    scheduler_.Set(&scheduler);
    timer_.Reset();

    Call calls[5];
    Call* call = &calls[1];
    auto scheduled = scheduler.RunEveryMicros(
      100, [&call]() { call->Make(); ++call; });
    scheduler.RunAfterMicros(200, [&]() {
      scheduler.RunEveryMicrosUntil(10, [&]() {
        if (calls[2]) {
          scheduler.Cancel(scheduled);
          return true;
        } else {
          return false;
        }
      });
    });
    scheduler.Loop();
      
    AssertInRange(calls[1].time(), 100, 105);
    AssertInRange(calls[2].time(), 200, 205);
    assert(!calls[3]);
  }

  {
    volatile Scheduler scheduler;
    scheduler_.Set(&scheduler);
    timer_.Reset();

    Call calls[5];
    TaskId scheduled[5] = {
      scheduler.RunAfterMicros(100, [&calls]() { calls[1].Make(); }),
      scheduler.RunAfterMicros(100, [&calls]() { calls[0].Make(); }),
      scheduler.RunAfterMicros(300, [&calls]() { calls[2].Make(); }),
      scheduler.RunAfterMicros(300, [&calls]() { calls[0].Make(); }),
      scheduler.RunEveryMicros(200, [call = calls + 3]() mutable { (call++)->Make(); })
    };
    scheduler.RunAfterMicros(0, [&]() { scheduler.Cancel(scheduled[1]); });
    scheduler.RunAfterMicros(0, [&]() { scheduler.Cancel(scheduled[3]); });
    scheduler.RunAfterMicros(250, [&]() { scheduler.Cancel(scheduled[4]); });
    scheduler.Loop();

    assert(!calls[4]);
    assert(!calls[0]);
    AssertInRange(calls[1].time(), 100, 105);
    AssertInRange(calls[2].time(), 300, 305);
    AssertInRange(calls[3].time(), 200, 210);
  }

  {
    volatile Scheduler scheduler;
    scheduler_.Set(&scheduler);
    timer_.Reset();

    Call calls[5];
    for (int i : {2, 1, 4, 3}) {
      scheduler.AfterMicros(i*100).ThenVoid([&calls, i]() { calls[i].Make(); });
    }
    scheduler.Loop();

    AssertInRange(calls[1].time(), 100, 105);
    AssertInRange(calls[2].time(), 200, 205);
    AssertInRange(calls[3].time(), 300, 310);
    AssertInRange(calls[4].time(), 400, 410);
  }
  
  {
    volatile Scheduler scheduler;
    scheduler_.Set(&scheduler);
    timer_.Reset();

    Call calls[5];
    scheduler.RunEveryMicrosUntilResolved(
      100, [&calls, &scheduler, i = 1]() mutable {
        if (i <= 2) {
          calls[i++].Make();
          return false;
        } else {
          return true;
        }
      })
      .ThenVoid([&calls]() { calls[0].Make(); });
    scheduler.Loop();

    assert(!calls[3]);
    AssertInRange(calls[1].time(), 100, 105);
    AssertInRange(calls[2].time(), 200, 205);
    AssertInRange(calls[0].time(), 300, 310);
  }

  return 0;
}
