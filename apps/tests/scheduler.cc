
#include "Arduino.h"

#include "os/scheduler.h"


Scheduler scheduler;


int main(void) {
  init();  // wiring.c
  Serial.begin(9600);

  uint32_t micros[10];
  scheduler.RunEveryMicrosUntil(10, [&micros, i = 0]() {
    if (i < 10) {
      micros[i++] = ::micros();
      return false;
    } else {
      return true;
    }
  });
  
  for (uint8_t i = 0; i < 10; ++i) {
    Serial.print(micros[i]);
  }
  Serial.flush();

  return 0;
}


// TODO: Remove.
void yield() {}

