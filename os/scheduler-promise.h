// Part of os/scheduler.h implementation that requires Promise to be defined.

#include "lib/promise.h"
#include "os/scheduler.h"

template <typename DescriptionT>
Promise<void> Scheduler<DescriptionT>::AfterMicros(uint32_t micros) volatile {
  PromiseWithResolve<void> promise;
  RunAfterMicros(micros, [promise]() mutable { promise.Resolve(); },
                 P("Resolve() AfterMicros()"));
  return promise;
}
