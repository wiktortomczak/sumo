#!/bin/bash
#
# Compiles C++ source code for the AVR microcontroller.

set -eumo pipefail

source $(dirname $0)/config.sh
source build/util.sh


# Compiles C++ source code for the AVR microcontroller.
#
# @param  $1 Path to .cc file (compilation unit) to compile.
# @param  $2..$n-1 Arbitrary flags to pass to the GCC compiler.
# @param  $n Build output directory (implies -c) or GCC output flag (-S or -E).
# @return Path to file with executable code in .elf format.
function compile() {
  local src=$1
  local output="${@: -1}"
  local -a extra_flags=("${@:2:$#-2}")
  
  log "compile $src"

  if ! [[ $output == -* ]]; then
    # $output is build output directory (build_dir)
    local out=${output}/$(replace_extension $src ".o")
    mkdir -p $(dirname $out)
    output="-c"
  else
    :  # $output is a flag eg. -E. No output file.
  fi

  $ARDUINO_BIN_DIR/avr-g++  \
    -Wall  \
    -Os  \
    -std=c++17  \
    $output  \
    -ffunction-sections -fdata-sections  \
    -mmcu=${GCC_AVR_MCU}  \
    -DF_CPU=$((${AVR_CPU_FREQ_MHZ} * 1000000))  \
    $(prepend_each "-I" ${INCLUDE_DIRS[@]})  \
    ${extra_flags[@]:-}  \
    $src  \
    -o ${out:-"/dev/stdout"}

  # TODO: Are these flags needed?
  # -isystem ${ARDUINO_DIR}/hardware/tools/avr/avr/include
  # -DARDUINO=160
  # -DARDUINO_ARCH_AVR
  # -D__PROG_TYPES_COMPAT__

  [[ -v out ]] && echo $out || true
}

INCLUDE_DIRS=(
  $REPOSITORY_DIR
  ${ARDUINO_INCLUDE_DIRS[@]}
)


# Compiles C++ source code for the AVR microcontroller.
# Pulls in the AVR port of C++ Standard Library.
# The code can include C++ library headers.
#
# See compile() for parameters and return value.
#
# TODO: Merge with compile() when the STL port is fixed not to cause
# compilation errors of Arduino IDE C++ libraries and other third-party code.
# Currenty, Arduino IDE libraries can only be compiled with compile().
function compile_libstdcxx() {
  local src=$1
  local output="${@: -1}"
  local -a extra_flags=("${@:2:$#-2}")

  local -a libstdcxx_flags=(
    $(prepend_each "-isystem" ${LIB_STDCCX_LIBC6_LIBC_INCLUDE_DIRS[@]})
    -D__x86_64__  # TODO: Remove. This define is incorrect for AVR.
  )

  compile $src ${libstdcxx_flags[@]} ${extra_flags[@]:-} $output
}

LIBSTDCXX_7_DIR=${REPOSITORY_DIR}/third_party/libstdc++-7-dev-7.3.0
LIBC6_DIR=${REPOSITORY_DIR}/third_party/libc6-dev-2.27
LINUX_LIBC_DIR=${REPOSITORY_DIR}/third_party/linux-libc-dev-4.4.0
LIB_STDCCX_LIBC6_LIBC_INCLUDE_DIRS=(
  ${LIBSTDCXX_7_DIR}/usr/include/c++/7
  ${LIBSTDCXX_7_DIR}/usr/include/c++/7/backward
  ${LIBSTDCXX_7_DIR}/usr/include/avr/c++/7
  ${LIBSTDCXX_7_DIR}/usr/include/x86_64-linux-gnu/c++/7
  ${LIBC6_DIR}/usr/include
  ${LIBC6_DIR}/usr/include/avr
  ${LIBC6_DIR}/usr/include/x86_64-linux-gnu
  ${LINUX_LIBC_DIR}/usr/include
)


if [[ $0 == ${BASH_SOURCE[0]} ]]; then  # Executed directly, not sourced.
  compile_libstdcxx "${@}"
fi
