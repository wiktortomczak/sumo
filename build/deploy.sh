#!/bin/bash
#
# Deploys AVR machine code to the AVR microcontroller.

set -eumo pipefail

source $(dirname $0)/config.sh
source build/util.sh


# Deploys AVR machine code to the AVR microcontroller.
#
# @param  $1 Path to file with AVR executable code in .elf format.
function deploy() {
  local image_elf=$1
  local image_ihex=$(replace_extension ${image_elf} ".ihex")

  log "deploy $image_ihex"
  
  # Extract .ihex file.
  $ARDUINO_BIN_DIR/avr-objcopy -O ihex -R .eeprom ${image_elf} ${image_ihex}

  # Upload .ihex file to the 
  $ARDUINO_BIN_DIR/avrdude  \
    -C $ARDUINO_DIR/hardware/tools/avr/etc/avrdude.conf  \
    -p $AVRDUDE_AVR_MCU  \
    -c arduino  \
    -P ${AVR_PORT}  \
    -U flash:w:${image_ihex}:i
}


if [[ $0 == ${BASH_SOURCE[0]} ]]; then  # Executed directly, not sourced.
  deploy $1
fi
