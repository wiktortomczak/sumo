#pragma once

#ifndef TEST_TIMER
#include "os/timer.h"
inline volatile Timer timer;  // Global Timer instance.
#else
auto& timer = TEST_TIMER;  // Injected global Timer.
#endif
