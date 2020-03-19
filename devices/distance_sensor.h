
#pragma once

#include <string_view>
#include <utility>

#include "lib/promise.h"
#include "lib/stream.h"
#include "os/pin.h"
#include "os/scheduler.h"


// Reads distance measured by a HC-SR04 ultrasonic sensor.
// Timer-based, not interrupt-based: repeatedly polls for pin state change
// to measure distance. Drives the sensor - device driver.
class DistanceSensor {
public:
  DistanceSensor(std::string_view id, OutputPin trig_pin, InputPin echo_pin)
    : id_(id), trig_pin_(trig_pin), echo_pin_(echo_pin) {
  }

  // Human-readable sensor ID.
  const std::string_view id() const { return id_; }

  // A distance reading from the sensor.
  struct Reading {
    uint16_t distance_mm;  // Measured distance from the sensor, in mm.
    uint32_t time_usec;    // Time the measurement was started, in usec.
  };

  // Continuously measures distance. Returns a stream of distance readings.
  Stream<Reading> StreamDistanceReadings() {
    Stream<Reading> readings;
    trig_pin_.SetState(PinState::LOW);
    scheduler.RunAfterMicros(2, [this, readings]() mutable {
      ReadDistances(std::move(readings));
    });
    return readings;
  }

private:
  void ReadDistances(Stream<Reading> readings) {
    ReadDistance().Then<void>(
      [this, readings = std::move(readings)](const Reading& reading) mutable {
        readings.Put(reading);
        ReadDistances(std::move(readings));
      });
  }

  Promise<Reading> ReadDistance() {
    // TODO: assert echo pin low.
    trig_pin_.SetState(PinState::HIGH);
    return scheduler.AfterMicros(10).Then<Reading>([this]() {
      trig_pin_.SetState(PinState::LOW);
      const uint32_t time_usec = timer.Now();
      return echo_pin_.OnceSpikes<POLL_FREQUENCY_USEC>().Then<Reading>(
        [time_usec](uint32_t echo_pin_spike_duration_usec) {
          const uint16_t distance_mm =
            echo_pin_spike_duration_usec * DISTANCE_MM_PER_MEASUREMENT_USEC;
          return Reading{distance_mm, time_usec};
        });
    });
  }

  const std::string_view id_;
  OutputPin trig_pin_;
  const InputPin echo_pin_;

  static constexpr float SOUND_SPEED_M_PER_SEC = 343;

  // Distance measurement time-to-distance coefficient.
  static constexpr float DISTANCE_MM_PER_MEASUREMENT_USEC =
    SOUND_SPEED_M_PER_SEC / 1000 / 2;

  // Polling frequency. However, the actual time between polls is likely to be
  // higher, as the underlying scheduler might be busy running other tasks.
  static constexpr uint32_t POLL_FREQUENCY_USEC = 50;
  // At 1 / 50 usec frequency, if the reading is off by max 50 usec,
  // the distance measurement error due to this driver is max 8.5mm.
};
