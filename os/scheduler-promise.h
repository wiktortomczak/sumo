// Part of os/scheduler.h implementation that requires Promise to be defined.

#include "lib/promise.h"
#include "os/scheduler.h"

template <bool has_stop>
Promise<void> Scheduler<has_stop>::AfterMicros(uint32_t micros) {
  PromiseWithResolve<void> promise;
  RunAfterMicros(micros, [promise]() mutable { promise.Resolve(); });
  return promise;
}
