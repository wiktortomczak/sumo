
#pragma once

#include "lib/check.h"
#include "lib/log.h"
#include "os/arduino.h"
#include "lib/promise.h"
#include "os/timer-global.h"


class InputPin;
class OutputPin;

// Interface to a single pin on the Arduino board.
class Pin {
public:
  // Creates a read-only instance, that sets the pin in input mode.
  static InputPin Input(uint8_t pin);

  // Creates a write-only instance, that sets the pin in output mode.
  static OutputPin Output(uint8_t pin);

  using State = ::PinState;
  
protected:
  Pin(uint8_t pin) : pin_(pin) {}
  const uint8_t pin_;
};


class InputPin : public Pin {
public:
  InputPin(uint8_t pin) : Pin(pin) {
    Arduino::SetDigitalPinMode(pin, PinMode::INPUT);
  }

  State GetState() const {
    return Arduino::GetDigitalPinState(pin_);
  };

  bool IsHigh() const {
    return static_cast<bool>(GetState());
  }

  bool IsLow() const {
    return !static_cast<bool>(GetState());
  }

  // TODO until the pin goes from low to high.
  // The returned promise is resolved when the pin goes high.
  template <uint32_t poll_frequency_usec>
  Promise<uint32_t> OnceGoesHigh() const {
    CHECK(IsLow());
    return OnceChanges([this](uint32_t micros) {
      CHECK(IsHigh());
      DLOG(INFO) << P("pin=") << pin_ << P(" HIGH");
      return micros;
    });
  }

  // TODO until the pin goes from low to low.
  // The returned promise is resolved when the pin goes low.
  template <uint32_t poll_frequency_usec>
  Promise<uint32_t> OnceGoesLow() const {
    CHECK(IsHigh());
    return OnceChanges([this](uint32_t micros) {
      CHECK(IsLow());
      DLOG(INFO) << P("pin=") << pin_ << P(" LOW");
      return micros;
    });
  }

  // Repeatedly polls the pin, via periodic scheduler tasks, until it spikes:
  // goes from low to high then from high to low. The returned promise is
  // resolved, with the spike duration in usec, after the two pin state changes
  // occur.
  template <uint32_t poll_frequency_usec>
  Promise<uint32_t> OnceSpikes() const {
    return OnceGoesHigh<poll_frequency_usec>().template Then<uint32_t>(
      [this](uint32_t time_high_usec) {
      return OnceGoesLow<poll_frequency_usec>().template Then<uint32_t>(
        [time_high_usec](uint32_t time_low_usec) {
          const uint32_t spike_duration_usec = time_low_usec - time_high_usec;
          return spike_duration_usec;
        });
    });
  }

  template <typename F>
  Promise<uint32_t> OnceChanges(F&& f) {
    // return pin_monitor.OnceChanges(pin_, std::move(f));
  }
};

inline InputPin Pin::Input(uint8_t pin) { return InputPin(pin); }


class OutputPin : public Pin {
public:
  OutputPin(uint8_t pin) : Pin(pin) {
    Arduino::SetDigitalPinMode(pin, PinMode::OUTPUT);
  }

  void SetState(State state) {
    Arduino::SetDigitalPinState(pin_, state);
  };

  void SetHigh() {
    SetState(PinState::HIGH);
  }

  void SetLow() {
    SetState(PinState::LOW);
  }
};

inline OutputPin Pin::Output(uint8_t pin) { return OutputPin(pin); }
