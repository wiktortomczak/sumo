
#pragma once

#include "os/arduino.h"


// Monotonic clock. Very rudimentary analog of C++ system_clock.
class Timer {
public:
  // TODO: Pass / return C++ duration<T>.
  uint32_t Now() {
    return Arduino::GetMicrosecondsSinceStart();
  }
};
