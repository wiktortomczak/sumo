
#include "Arduino.h"


struct InterruptsDisabled {
  InterruptsDisabled() : sreg_copy_(SREG) { cli(); }
  ~InterruptsDisabled() { SREG = sreg_copy_; }
  const uint8_t sreg_copy_;
};


constexpr uint8_t microseconds_per_timer0_tick = 64 / 16;

extern volatile unsigned long timer0_overflow_count;

uint32_t Micros() {
  uint32_t num_timer0_ticks_hi;  // *256
  uint8_t num_timer0_ticks_low;  // *1

  {
    InterruptsDisabled d;
    num_timer0_ticks_hi = timer0_overflow_count;
    num_timer0_ticks_low = TCNT0;  // TODO: TCNT0L?
  }

  return (
    ((num_timer0_ticks_hi << 8) + num_timer0_ticks_low)
    * microseconds_per_timer0_tick);
};


int main(void) {
  init();  // wiring.c

  uint32_t micros[10];
  uint32_t Micros[10];
  for (uint8_t i = 0; i < 10; ++i) {
    micros[i] = ::micros();
    Micros[i] = ::Micros();
  }

  Serial.begin(9600);
  for (uint8_t i = 0; i < 10; ++i) {
    char buf[16];
    snprintf(buf, sizeof(buf), "%04lu %04lu\n", micros[i], Micros[i]);
    Serial.write(buf);
  }  
  Serial.flush();

  return 0;
}


// TODO: Remove.
void yield() {}

