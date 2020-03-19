#!/bin/bash

# Builds AVR machine code from C++ source code and deploys it to an
# AVR microcontroller.
# See top-level README.me#Building-the-code .
#
# Based on the following sources:
# https://stackoverflow.com/questions/32413959/avr-gcc-with-arduino
# https://gist.github.com/mulderp/36ca39a9911d54de66ec
# http://thinkingonthinking.com/an-arduino-sketch-from-scratch/
#
# To allow non-root access to $AVR_PORT on Ubuntu:
# $ sudo usermod -a -G dialout $USER; 
# $ /sbin/reboot

set -eumo pipefail

source build/build.sh
source build/deploy.sh


# @param  $1 Path to .cc file (compilation unit) to build.
main() {
  local src=$1
  deploy $(build $src)
}


main $1
