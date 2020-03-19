#!/bin/bash
#
# Builds and runs automated tests of source code in development environment
# - *not* in the AVR microcontroller.

set -eumo pipefail

source $(dirname $0)/config.sh
source build/compile.sh
source build/util.sh


# Builds and runs automated tests of source code in development environment.
#
# @param $1..$n Paths to .cc source files with test programs.
#   If no paths are given, runs all tests found in the repository.
function main() {
  if [[ $@ ]]; then
    local -a tests=( $@ )
  else
    local -a tests=(
      $(find -name third_party -prune -o -name '*test.cc' -print | cut -c3-)
    )
  fi
  build_run_tests ${tests[@]}
}

function build_run_tests() {
  local -a tests=($@)
  for test_src in ${tests[@]}; do
    echo $test_src

    # Build the test.
    test_bin="out/$(strip_extension $test_src)"
    mkdir -p $(dirname $test_bin)
    # TODO: Generalize and reuse compile().
    g++  \
      -std=c++17 -Wall -O0 -g  \
      $(prepend_each "-I" ${INCLUDE_DIRS[@]})  \
      $test_src -o $test_bin

    # Run the test.
    $test_bin && echo "$test_src passed" || echo "$test_src failed"
  done
}


if [[ $0 == ${BASH_SOURCE[0]} ]]; then  # Executed directly, not sourced.
  main $@
fi
