
#pragma once

#ifndef TEST_SCHEDULER
#include "arduino-ext/critical_section.h"
#include "arduino-ext/pgm.h"
#include "os/scheduler.h"
// Global Scheduler instance.
inline volatile Scheduler<const PGM<char>> scheduler;
#else
volatile auto& scheduler = TEST_SCHEDULER;  // Injected global Scheduler.
#endif
