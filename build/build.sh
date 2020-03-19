#!/bin/bash
#
# Builds AVR machine code from C++ source code.

set -eumo pipefail

source $(dirname $0)/config.sh
source build/compile.sh
source build/util.sh


# Builds AVR executable machine code from C++ source code.
# Links against selected core libs from Arduino IDE: wiring, serial.
#
# @param  $1 Path to .cc file (compilation unit) to build.
# @return Path to file with executable code in .elf format.
function build() {
  local src=$(realpath --relative-to=$REPOSITORY_DIR $1)
  local label=$(strip_extension $(basename $src))
  local build_dir="out/${label}.build"

  log "build $src in $build_dir"

  rm -rf ${build_dir}
  mkdir -p ${build_dir}

  local libwiring=$(build_libwiring ${build_dir})
  local libserial=$(build_libserial ${build_dir})
  # local o=$(compile_libstdcxx ${src} $build_dir)
  local o=$(compile ${src} $build_dir)
  local elf=$(link $o ${libwiring} ${libserial})

  echo $elf
}

CORES_DIR=$ARDUINO_DIR/hardware/arduino/avr/cores/arduino

function build_libwiring() {
  local build_dir=$1
  srcs=(
    $CORES_DIR/wiring.c
    $CORES_DIR/wiring_digital.c
    $CORES_DIR/wiring_pulse.c
    # $CORES_DIR/wiring_pulse.S  # TODO: Is this source file needed?
  )
  build_library "wiring" ${srcs[@]} $build_dir
}

function build_libserial() {
  local build_dir=$1
  srcs=(
    $CORES_DIR/HardwareSerial.cpp
    $CORES_DIR/HardwareSerial0.cpp
    $CORES_DIR/Print.cpp
  ) 
  build_library "serial" ${srcs[@]} $build_dir
}

# Builds AVR machine code library from C++ source code.
#
# @param  $1 Library name. Arbitrary name for the output library.
# @param  $2..$n-1 Paths to .cc files to compile and package into the library.
# @param  $n Build output directory. Where to output the library .a file.
# @return Path to the library .a file.
function build_library() {
  local library=$1
  local build_dir="${@: -1}"
  local -a srcs=("${@:2:$#-2}")

  local os=()
  for src in "${srcs[@]}"; do
    o=$(compile $src $build_dir)
    os=(${os[@]:-} $o)
  done

  local a=${build_dir}/lib${library}.a  # TODO: In src-dependent directory.
  $ARDUINO_BIN_DIR/avr-ar rcs $a ${os[@]}

  echo $a
}


# Links object file into an executable.
#
# @param  $1 Path to object file.
# @param  $2..$n Paths to library files to link against.
# @return Path to file with executable code in .elf format.
function link() {
  local o=$1
  local -a libs=("${@:2}")
  
  log "link ${@}"

  local elf=$(replace_extension $o ".elf")

  $ARDUINO_BIN_DIR/avr-g++  \
    -mmcu=${GCC_AVR_MCU}  \
    -Wl,--gc-sections  \
    -Os  \
    $o ${libs[@]}  \
    -o $elf

  # TODO:
  # -lc
  # -lm

  echo $elf
}


if [[ $0 == ${BASH_SOURCE[0]} ]]; then  # Executed directly, not sourced.
  build $1
fi
