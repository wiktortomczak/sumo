#!/bin/bash
#
# Build script configuration. Common defines used by scripts in this package.

set -eumo pipefail

REPOSITORY_DIR=$(realpath $(dirname $0)/..)
# Change the directory to repository root so that all paths are unambiguous.
cd $REPOSITORY_DIR

# TODO: Explicitly list / check for all external dependencies.

ARDUINO_DIR=third_party/arduino-1.8.12
ARDUINO_BIN_DIR=$ARDUINO_DIR/hardware/tools/avr/bin
ARDUINO_INCLUDE_DIRS=(
  $ARDUINO_DIR/hardware/arduino/avr/cores/arduino
  $ARDUINO_DIR/hardware/arduino/avr/variants/standard
)

# AVR microcontroller type (part name). Tool-specific labels.
AVRDUDE_AVR_MCU=m328p
GCC_AVR_MCU=atmega328p

# AVR CPU frequency.
AVR_CPU_FREQ_MHZ=16

# Linux machine port to which the AVR microcontroller is connected.
AVR_PORT=${PORT:-"/dev/ttyUSB0"}
