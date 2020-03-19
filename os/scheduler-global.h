
#pragma once

#ifndef TEST_SCHEDULER
#include "os/scheduler.h"
inline Scheduler scheduler;  // Global Scheduler instance.
#else
auto& scheduler = TEST_SCHEDULER;  // Injected global Scheduler.
#endif

