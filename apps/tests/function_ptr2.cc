
#include "Arduino.h"


template <int N>
void Print();

using VoidFunctionPtr = void(*)(void);
VoidFunctionPtr funcs[2] = {&Print<1>, &Print<2>};

int main(void) {
  init();  // wiring.c

  Serial.begin(9600);

  for (int i = 0; i < 2; i++) {
    funcs[i]();
  }

  return 0;
}

template <int N>
void Print() {
  Serial.println(N);
  Serial.flush();
}

// TODO: Remove.
void yield() {}

