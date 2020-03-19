// An Arduino board program that continuously reads distance sensors
// and writes the distance readings to the on-board USB port.

#include <array>

#include "devices/distance_sensor.h"
#include "os/scheduler-global.h"

#include "Arduino.h"

using namespace std;


// Top-level class. Represents the robot (the board application),
// its functionality and hardware.
class Robot {
public:
  // Runs the robot. Registers actions to be executed in the scheduler.
  void Run() {
    for (const DistanceSensor& sensor : sensors_ ) {
      sensor.StreamDistanceReadings().ThenEvery(
        [this, &sensor](const DistanceSensor::Reading& reading) {
          WriteDistanceReadingToUSB(sensor, reading);
        });
    }
  }

private:
  // Writes a distance reading to the on-board USB.
  void WriteDistanceReadingToUSB(
    const DistanceSensor& sensor,
    const DistanceSensor::Reading& reading) {
    char buf[64];
    snprintf(buf, sizeof(buf), "sensor=%s time=%u distance=%u\n",
             sensor.id().c_str(), reading.time_usec, reading.distance_mm);
    Serial.write(buf);
    Serial.flush();
  }

  // Peripherals.
  array<DistanceSensor, 1> sensors_ = {
    DistanceSensor("front", 2, 3),
    DistanceSensor("right", 4, 5),
    DistanceSensor("back", 6, 7),
    DistanceSensor("left", 8, 9)
  };
};


int main() {
  // Init Arduino IDE libraries.
  init();  // wiring.c
  Serial.begin(9600);

  Robot robot;
  robot.Run();

  scheduler.Loop();  // Does not return.
  return 0;
}
