
#include "Arduino.h"


int main(void) {
  init();  // wiring.c

  Serial.begin(9600);

  for (uint8_t i = 0; i < 10; ++i) {
    Serial.write(i);
  }
  Serial.flush();

  return 0;
}


// TODO: Remove.
void yield() {}

