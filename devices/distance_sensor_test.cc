
#include <list>
#include <map>

using namespace std;

#include "os/testing/fake_timer.h"


enum class PinState : uint8_t;
enum class PinMode : uint8_t;

struct FakePin {
  struct Call {
    uint32_t time;
    PinState state;

    bool operator==(const Call& other) {
      return time == other.time && state == other.state;
    }
  };
  struct Calls {
    bool empty() const { return calls_.empty(); }
    Call pop_front() {
      const Call call = calls_.front();
      calls_.pop_front();
      return call;
    }
    list<Call> calls_;
  } calls;

  void SetState(PinState state_) {
    state = state_;
  }

  PinState state;
};

class FakeArduino {
public:
  static PinState GetDigitalPinState(uint8_t pin) {
    return pins_[pin].state;
  }

  static void SetDigitalPinState(uint8_t pin, PinState state) {
    FakePin::Call call{timer_.Now(), state};
    pins_[pin].calls.calls_.push_back(call);
  }

  static void SetDigitalPinMode(uint8_t pin, PinMode mode) {}

  static FakePin& pin(int i) { return pins_[i]; }

private:
  static map<int, FakePin> pins_;
};

map<int, FakePin> FakeArduino::pins_;

#define TEST_ARDUINO FakeArduino  // Inject FakeArduino into code under test.


#include "os/testing/test_scheduler.h"
#include "devices/distance_sensor.h"
#include "lib/testing/streams.h"
#include "os/pin.h"


int main() {
  {
    DistanceSensor sensor1("sensor-1", OutputPin(1), InputPin(2));
    FakePin& sensor1_trig_pin = FakeArduino::pin(1);
    FakePin& sensor1_echo_pin = FakeArduino::pin(2);
    auto sensor1_readings = Streams::ToVector(sensor1.StreamDistanceReadings());

    DistanceSensor sensor2("sensor-2", OutputPin(3), InputPin(4));
    FakePin& sensor2_trig_pin = FakeArduino::pin(3);
    FakePin& sensor2_echo_pin = FakeArduino::pin(4);
    auto sensor2_readings = Streams::ToVector(sensor2.StreamDistanceReadings());

    DistanceSensor sensor3("sensor-3", OutputPin(5), InputPin(6));
    FakePin& sensor3_trig_pin = FakeArduino::pin(5);
    auto sensor3_readings = Streams::ToVector(sensor3.StreamDistanceReadings());

    // Program responses from fake echo pins.
    scheduler.RunAfterMicros(1000, [&]() {
      sensor1_echo_pin.SetState(PinState::HIGH); });
    scheduler.RunAfterMicros(2000, [&]() {
      sensor1_echo_pin.SetState(PinState::LOW); });

    scheduler.RunAfterMicros(2400, [&]() {
      sensor1_echo_pin.SetState(PinState::HIGH); });
    scheduler.RunAfterMicros(2800, [&]() {
      sensor1_echo_pin.SetState(PinState::LOW); });

    scheduler.RunAfterMicros(1200, [&]() {
      sensor2_echo_pin.SetState(PinState::HIGH); });
    scheduler.RunAfterMicros(2400, [&]() {
      sensor2_echo_pin.SetState(PinState::LOW); });
    
    scheduler.Loop(3000);

    assert(*sensor1_readings
           == vector<DistanceSensor::Reading>({{171, 16}, {68, 2028}}));
    assert(sensor1_trig_pin.calls.pop_front()
           == (FakePin::Call{0, PinState::LOW}));
    // 2ms after LOW, but timer / scheduler resolution is 4ms.
    assert((sensor1_trig_pin.calls.pop_front())
           == (FakePin::Call{4, PinState::HIGH}));
    // 10ms after HIGH, but timer / scheduler resolution is 4ms.
    assert((sensor1_trig_pin.calls.pop_front())
           == (FakePin::Call{16, PinState::LOW}));
    assert((sensor1_trig_pin.calls.pop_front())
           // sensor1_echo_pin goes low at t=2000, next poll is at t=2016.
           == (FakePin::Call{2016, PinState::HIGH}));
    assert((sensor1_trig_pin.calls.pop_front())
           == (FakePin::Call{2028, PinState::LOW}));
    assert((sensor1_trig_pin.calls.pop_front())
           // sensor1_echo_pin goes low at t=2800, next poll is at t=2828.
           == (FakePin::Call{2828, PinState::HIGH}));
    assert((sensor1_trig_pin.calls.pop_front())
           == (FakePin::Call{2840, PinState::LOW}));
    assert(sensor1_trig_pin.calls.empty());

    assert(*sensor2_readings == vector<DistanceSensor::Reading>({{205, 16}}));
    assert(sensor2_trig_pin.calls.pop_front()
           == (FakePin::Call{0, PinState::LOW}));
    assert((sensor2_trig_pin.calls.pop_front())
           == (FakePin::Call{4, PinState::HIGH}));
    assert((sensor2_trig_pin.calls.pop_front())
           == (FakePin::Call{16, PinState::LOW}));
    assert((sensor2_trig_pin.calls.pop_front())
           // sensor2_echo_pin goes low at t=2400, next poll is at t=2416.
           == (FakePin::Call{2416, PinState::HIGH}));
    assert((sensor2_trig_pin.calls.pop_front())
           == (FakePin::Call{2428, PinState::LOW}));
    assert(sensor2_trig_pin.calls.empty());

    assert(sensor3_readings->empty());
    assert(sensor3_trig_pin.calls.pop_front()
           == (FakePin::Call{0, PinState::LOW}));
    assert((sensor3_trig_pin.calls.pop_front())
           == (FakePin::Call{4, PinState::HIGH}));
    assert((sensor3_trig_pin.calls.pop_front())
           == (FakePin::Call{16, PinState::LOW}));
    assert(sensor3_trig_pin.calls.empty());
  }

  return 0;
}


bool operator==(const DistanceSensor::Reading& a,
                const DistanceSensor::Reading& b) {
  return a.distance_mm == b.distance_mm && a.time_usec == b.time_usec;
}
