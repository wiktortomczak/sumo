
#include "Arduino.h"

int main() {
  init();

  pinMode(LED_BUILTIN, OUTPUT);
  for (uint8_t counter = 0;; ++counter) {
    digitalWrite(LED_BUILTIN, HIGH);
    delay(1000);
    digitalWrite(LED_BUILTIN, LOW);
    delay(1000);
  }

  return 0;
}


// TODO: Remove.
void yield() {}
