
#include <functional>

#include "testing/Arduino.h"

int main() {
  init();

  pinMode(LED_BUILTIN, OUTPUT);
  std::function f([]() {
    digitalWrite(LED_BUILTIN, HIGH);
    delay(1000);
    digitalWrite(LED_BUILTIN, LOW);
    delay(1000);
  });

  for (uint8_t counter = 0;; ++counter) {
    f();
  }

  return 0;
}

namespace std {

void __throw_bad_function_call() {
  
}

}  // namespace std


extern "C" {
// TODO: Remove.
void yield() {}
}
