
#pragma once

#include <array>
#include <functional>
#include <avr/interrupt.h>  // TODO: Wrap in arduino-core/*, os/arduino.h.

#include "arduino-core/interrupt.h"
#include "arduino-core/wiring.h"
#include "arduino-ext/critical_section.h"
#include "os/arduino.h"
#include "os/scheduler_executor-global.h"
#include "os/timer-global.h"


class PinMonitor {
public:
  static constexpr size_t MAX_PINS = 4;

  template <size_t... pins>
  void Init() volatile {
    this_nv()->monitored_pins_ = {pins...};
    this_nv()->num_pins_ = sizeof...(pins);
    
    (Arduino::SetDigitalPinMode(pins, PinMode::INPUT), ...);
    CRITICAL_SECTION({
      ((this_nv()->pin_states_[this_nv()->PinId(pins)] =
        Arduino::GetDigitalPinState(pins)), ...);
      (EnablePinChangeInterrupt<pins>(), ...);
    })
  }

  template <typename CallbackT>
  void OnceChanges(uint8_t pin, CallbackT&& f) volatile {
    // TODO: CHECK that the two methods: OnceChanges(), HandlePinStateSnapshot()
    // that modify change_callbacks are called in the same thread, so never
    // concurrently.
    auto& callback = this_nv()->change_callbacks_[this_nv()->PinId(pin)];
    CHECK(!callback);
    callback = f;
  }

  using callback_t = std::function<void(PinState, uint32_t)>;

private:
  template <size_t pin>
  static void EnablePinChangeInterrupt();

  struct PinStateSnapshot {
    uint32_t micros;
    PinState pin_states_new[MAX_PINS];
  };

public:  // TODO: private
  void HandlePinChangeInterrupt() volatile {
    PinStateSnapshot snapshot;
    snapshot.micros = timer.Now();

    // Read pin states. TODO: Batch read.
    for (uint8_t i = 0; i < num_pins_; ++i) {
      snapshot.pin_states_new[i] =
        Arduino::GetDigitalPinState(this_nv()->monitored_pins_[i]);
    }

    // TODO: CHECK(executor.is_single_threaded()).
    executor.RunAsync([this, snapshot]() {
      // RunAsync serializes HandlePinStateSnapshot() invocations,
      // so no risk of concurrency inside them.
      this_nv()->HandlePinStateSnapshot(snapshot);
    }, P("HandlePinStateSnapshot"));
  }

private:
  void HandlePinStateSnapshot(const PinStateSnapshot& snapshot) {
    for (uint8_t i = 0; i < num_pins_; ++i) {
      const PinState pin_state_new = snapshot.pin_states_new[i];
      if (pin_state_new != pin_states_[i]) {
        if (change_callbacks_[i]) {
          // Clear change_callbacks_[i] before calling it,
          // so that the callback can register another callback.
          callback_t callback;
          change_callbacks_[i].swap(callback);
          callback(pin_state_new, snapshot.micros);
        }
        pin_states_[i] = pin_state_new;
      }
    }
  };

  uint8_t PinId(uint8_t pin) const {
    for (uint8_t i = 0; i < num_pins_; ++i) {
      if (monitored_pins_[i] == pin) {
        return i;
      }
    }
    LOG(FATAL) << P("pin=") << pin << P(" not monitored");
  }

  PinMonitor* this_nv() const volatile {
    return const_cast<PinMonitor*>(this);
  }

  callback_t change_callbacks_[MAX_PINS];
  
  PinState pin_states_[MAX_PINS];

  std::array<uint8_t, MAX_PINS> monitored_pins_;  // TODO: const.
  size_t num_pins_;  // TODO: const.
};

// Global instance.
inline volatile PinMonitor pin_monitor;


ISR(PCINT0_vect) {
  Thread::Indicator interrupt_thread(Thread::Id::INTERRUPT);
  pin_monitor.HandlePinChangeInterrupt();
}

ISR(PCINT2_vect) {
  Thread::Indicator interrupt_thread(Thread::Id::INTERRUPT);
  pin_monitor.HandlePinChangeInterrupt();
}


#if !defined bit
#define bit(b) (1UL << (b))
#else
#error "bit() already defined"
#endif 
  
// Taken from https://gammon.com.au/interrupts. TODO and 'Table of pins'.
template <>
void PinMonitor::EnablePinChangeInterrupt<1>() {
  PCMSK2 |= bit(PCINT17);  // Set pin D1 to generate pin change interrupt #2.
  PCIFR |= bit(PCIF2);  // Clear any outstanding pin interrupt #2.
  PCICR |= bit(PCIE2);  // Enable pin change interrupt #2.
}

template <>
void PinMonitor::EnablePinChangeInterrupt<2>() {
  PCMSK2 |= bit(PCINT18);  // Set pin D2 to generate pin change interrupt #2.
  PCIFR |= bit(PCIF2);  // Clear any outstanding pin interrupt #2.
  PCICR |= bit(PCIE2);  // Enable pin change interrupt #2.
}

template <>
void PinMonitor::EnablePinChangeInterrupt<3>() {
  PCMSK2 |= bit(PCINT19);  // Set pin D3 to generate pin change interrupt #2.
  PCIFR |= bit(PCIF2);  // Clear any outstanding pin interrupt #2.
  PCICR |= bit(PCIE2);  // Enable pin change interrupt #2.
}

template <>
void PinMonitor::EnablePinChangeInterrupt<4>() {
  PCMSK2 |= bit(PCINT20);  // Set pin D4 to generate pin change interrupt #2.
  PCIFR |= bit(PCIF2);  // Clear any outstanding pin interrupt #2.
  PCICR |= bit(PCIE2);  // Enable pin change interrupt #2.
}

template <>
void PinMonitor::EnablePinChangeInterrupt<5>() {
  PCMSK2 |= bit(PCINT21);  // Set pin D5 to generate pin change interrupt #2.
  PCIFR |= bit(PCIF2);  // Clear any outstanding pin interrupt #2.
  PCICR |= bit(PCIE2);  // Enable pin change interrupt #2.
}

template <>
void PinMonitor::EnablePinChangeInterrupt<6>() {
  PCMSK2 |= bit(PCINT22);  // Set pin D6 to generate pin change interrupt #2.
  PCIFR |= bit(PCIF2);  // Clear any outstanding pin interrupt #2.
  PCICR |= bit(PCIE2);  // Enable pin change interrupt #2.
}

template <>
void PinMonitor::EnablePinChangeInterrupt<7>() {
  PCMSK2 |= bit(PCINT23);  // Set pin D7 to generate pin change interrupt #2.
  PCIFR |= bit(PCIF2);  // Clear any outstanding pin interrupt #2.
  PCICR |= bit(PCIE2);  // Enable pin change interrupt #2.
}

template <>
void PinMonitor::EnablePinChangeInterrupt<9>() {
  PCMSK0 |= bit(PCINT1);  // Set pin D9 to generate pin change interrupt #1.
  PCIFR |= bit(PCIF0);  // Clear any outstanding pin interrupt #1.
  PCICR |= bit(PCIE0);  // Enable pin change interrupt #1.
}

template <>
void PinMonitor::EnablePinChangeInterrupt<11>() {
  PCMSK0 |= bit(PCINT3);  // Set pin D11 to generate pin change interrupt #1.
  PCIFR |= bit(PCIF0);  // Clear any outstanding pin interrupt #1.
  PCICR |= bit(PCIE0);  // Enable pin change interrupt #1.
}
