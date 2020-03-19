
#pragma once

enum class PinState : uint8_t;
enum class PinMode : uint8_t;


#ifndef TEST_ARDUINO

#include "Arduino.h"

// Arduino hardware abstraction layer (HAL). Interface to on-board hardware.
//
// Implemented as a thin wrapper around Arduino.h bundled with Arduino IDE.
// 
// TODO: Remove this dependency and manipulate the board directly?
// Apparently functions in Arduino.h do a lot more than needed.
class Arduino {
public:
  static void SetDigitalPinMode(uint8_t pin, PinMode mode) {
    ::setPinMode(pin, mode);
  }

  static PinState GetDigitalPinState(uint8_t pin) {
    return ::digitalRead(pin);
  }

  static void SetDigitalPinState(uint8_t pin, PinState state) {
    ::digitalWrite(pin, state);
  }

  // TODO: Pass / return C++ duration<T>.
  static uint32_t GetMillisecondsSinceStart() {
    return ::millis();
  }

  // TODO: Pass / return C++ duration<T>. Merge with the above.
  static uint32_t GetMicrosecondsSinceStart() {
    return ::micros();
  }
};

#else

using Arduino = TEST_ARDUINO;

#endif


enum class PinState : uint8_t {
  LOW = 0,
  HIGH = 1
};

enum class PinMode : uint8_t {
  INPUT = 0,
  OUTPUT = 1
};
