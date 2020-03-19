
#include "Arduino.h"


void Print();

int main(void) {
  init();  // wiring.c

  Serial.begin(9600);

  using VoidFunctionPtr = void(*)(void);
  VoidFunctionPtr Print_ptr = &Print;
  Print_ptr();

  return 0;
}

void Print() {
  for (uint8_t i = 0; i < 10; ++i) {
    Serial.println(i);
  }
  Serial.flush();
}

// TODO: Remove.
void yield() {}

