# Sumo robot controller

This is the controller / software / logic layer of a custom-made robot
(TODO: general robot information) for [sumo robot competition](
https://en.wikipedia.org/wiki/Robot-sumo).

## Overview

### Controller

The software controller controls the robot's hardware (electronic parts) and
implements a particular behavior / combat strategy (TODO: strategy information).
Specifically, the software:

| *functionality* | *layer* |
| - | - |
| interfaces the robot's microcontroller and board:<br/>  pins, timer, interrupts, serial port, ...  | [OS](os/) |
| handles inputs from the robot's sensors (via board pins)<br/>generates commands for the robot's motors (via board pins) | [device driver](devices/) |
| implements rules that determine what motor commands to generate | [application](apps/controllers/) |

All executable code in the robot's microcontroller is this software.

### Hardware

* [Arduino Uno](https://en.wikipedia.org/wiki/Arduino_Uno) board
  with [ATmega328p](https://en.wikipedia.org/wiki/ATmega328) microcontroller
* distance sensors TODO
* surface sensors TODO
* motors TODO
* other TODO 

## Repository contents

* source code of the above [controller](#Controller)
   * including automated unit tests
* development tools
   * [build scripts](#Building-the-code)
   * [test scripts](#Testing-the-code)
   * [debugging tools](#Debugging-the-code)

### Directory organization

* `apps/` - source code of application layer<br/>
  (top-level microcontroller executables / programs / applications)
* `apps/controllers/` - robot controllers
* `apps/tests/` - test / experimental microcontroller programs, eg.
   * experiments with the board (eg. interrupts) / peripherals (eg. sensors)
   * microcontroller assembler code inspection
   * compilation tests
   * timing tests
* `build/` - [build scripts](#Building-the-code) & [test scripts](#Testing-the-code)
* `debugging/` - [debug scripts](#Debugging-the-code)
* `devices/` - soource code of device driver layer
* `lib/` - source code of own common libraries
* `os/` - soource code of OS layer
* `out/` - executables produced by the [build scripts](#Building-the-code) (generated directory)
* `*/testing/` - helper source code for writing tests
* `third_party/` - source code of third-party libraries

## Software details

### Platform specifics / constraints

[ATmega328p](https://en.wikipedia.org/wiki/ATmega328) is a microcontroller with:

* 16 MHz processor, 8-bit word
* 2KB RAM
* 32KB flash memory (storing code, read-only at runtime)

Implications for application code:

* Run-time memory usage must be under 2KB - absolutely minimal.
   * Static / preallocated fixed-size buffers should be preffered over dynamic
     memory allocation. Buffer sizes and static vs dynamic split should
     be fine-tuned to particular application's memory usage patterns.
   * Constants (eg. arrays of numbers) should be placed in flash memory.
* Compiled code must fit 32KB.
* Code interacts directly with the microcontroller (pins, interrupts).
  There is no pre-existing operating system that would provide abstraction
  over hardware.
* Code is single-threaded and does not need to implement thread safety measures
  (eg. locks), provided that interrupt handlers (the only source of asynchrony)
  do not call into it.

### Programming language

Standard modern [C++17](https://en.wikipedia.org/wiki/C%2B%2B17).

.. together with parts of [C++ standard library](https://en.wikipedia.org/wiki/C%2B%2B_Standard_Library)
(including STL), [ported to AVR](TODO), primarily for the following headers:</br>
`<functional>`, `<memory>`, `<utility>`, `<tuple>`

#### C++-specific platform implications

Mimimizing the application's run-time memory usage via:

* Custom memory allocator.
* No exceptions, no RTTI
  (see [`compile()`](build/compile.sh) for all compiler flags).
* Single compilation unit (`.cc` file) and header-only libraries (`.h` files).
   * to maximize compiler's code optimizations
   * multiple compilation units (`.cc` files) not needed as the problem they
     address - long (re)compilation times - does not occur
* Shared object dependencies stored in global inline variables rather than
  injected via pointer / reference.
   * In tests, dependencies are replaced with mocks / test equivalents via
     preprocessor macros.

### Architecture

TODO

## Software release process

### Environments

* development environment - a regular Linux machine
   * where the code is developed, built and pre-tested (compiled and run)
* target environment - [Arduino Uno](https://en.wikipedia.org/wiki/Arduino_Uno)
  with [ATmega328p](https://en.wikipedia.org/wiki/ATmega328) microcontroller
   * where the code is deployed and run
   
### Building the code

Building the code produces machine-executable code from source code, for either
of the above [environments](#Environments) / processors:

* for Arduino (target environment, the usual case) -
  to deploy and run the code in the Arduino board
* for Linux (development environment) -
  to pre-test the code by running it on the development machine

In either case, the building is done on the development machine.
An automated [build script](build/build.sh) runs the [GNU C/C++ compiler and linker](
https://gcc.gnu.org/) for all compilation units (while own code consists of
only [one compilation unit](#c-specific-platform-implications), some
third-party libraries introduce additional ones).

* When building for Linux (development environment), its distribution's standard
  GNU C/C++ compiler and linker are used.
* When building for Arduino (target environment), the `avr-gcc` version of
  GNU C/C++ compiler and linker are used. The `avr-gcc` version is configured as
  a cross-compiler for AVR processors and generates appropriate AVR machine code.

In a second step, when building for Arduino, an automated
[deploy script](build/deploy.sh) uploads the machine executable to the Arduino board.

The two scripts are wrapped in a single [build-deploy script](build/build-deploy.sh)
for convenience.

#### Example

```shell
# Prerequisite: Connect the board to the development machine.
$ cd <repository root>
$ build/build-deploy.sh apps/controllers/distance_sensors.cc
# The code is now running on the board, provided there was no error message.

# If the board is connected to a different port than /dev/ttyUSB0:
$ export AVR_PORT=/dev/<correct device>
$ build/build-deploy.sh apps/controllers/distance_sensors.cc
```

### Testing the code

* automated unit and larger tests, run in development environment<br/>
  with an automated [test script](build/test.sh)
* [test / experimental programs](apps/tests/), run in target environment

#### Example

```shell
cd <repository root>

# Build and run all automated tests.
$ build/test.sh   # Prints passing / fails tests.

# Build and run specific test(s).
$ build/test.sh os/scheduler_test.cc
```

### Inspecting AVR assembler code

The compile step of the build script can be run standalone and optionally produce
intermediate output. [`compile.sh`](build/compile.sh) optionally takes the standard
GCC output flag `-S` (assembler code) or `-E` (preprocessed source code).

#### Example

```
$ cd <repository root>
$ build/compile.sh apps/tests/blink.cc -S  # Assembler code goes to stdout
```

### Debugging the code

Run-time assertions in the code running in target environment (on the board),
removed in non-debug builds, that, when failed:

* signal the failure in a human-noticeable way
  (via an on-board LED / buzzer / etc.)
* log and buffer error messages, that can be later retrieved from the board
  to the development machine via the board's serial port, acting as debugging
  interface

## External dependencies

* [GNU Compiler Collection (GCC)](https://gcc.gnu.org)
* [avr-gcc](https://gcc.gnu.org/wiki/avr-gcc): GCC variant for AVR
* [Arduino IDE C/C++ libraries](https://github.com/arduino/Arduino)
