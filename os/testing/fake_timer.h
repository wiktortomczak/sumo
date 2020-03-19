
#pragma once

#include <cstdint>

// Fakes on-board Arduino timer, configured to tick every 4 usec
// (every 64 clock cycles) in the Arduino IDE code.
class FakeTimer {
public:
  uint32_t Now() {
    // Tick only every 100 calls (~ scheduler operations) so that
    // the scheduler can manage all its operations without racing,
    // allowing exact timing of events in test.
    if (++call_count_ == 100) {
      call_count_ = 0;
      now_ += 4;
    }
    return now_;
  };

private:                                 
  uint32_t now_ = 0;
  int call_count_ = 0;
} timer_;

#define TEST_TIMER timer_  // Inject timer_ into code under test.
