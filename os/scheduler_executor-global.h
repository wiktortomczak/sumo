
#pragma once

#ifndef TEST_EXECUTOR
#include "os/scheduler-global.h"
#include "os/scheduler_executor.h"
inline SchedulerExecutor executor;  // Default global Executor.
#else
auto& executor = TEST_EXECUTOR;  // Injected global Executor.
#endif
