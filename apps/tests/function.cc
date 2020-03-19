
#include <functional>

#include "testing/Arduino.h"


int main(void) {
  init();  // wiring.c
  Serial.begin(9600);

  std::function f([]() {
    Serial.println("f()");
    Serial.flush();
  });
  f();

  return 0;
}


// TODO: Remove.
void yield() {}
