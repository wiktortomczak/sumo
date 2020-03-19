#!/bin/bash
#
# Common functions and utilities.

set -eumo pipefail


# Logs a message to stderr.
#
# @param $1..$n message
function log() {
  echo "${@}" >&2
}


# Strips extension from file name.
#
# @param $1 File name ending with an extension: '.' followed by 0+ non-'.'s.
# @return File name without the extension.
function strip_extension() {
  echo "${1%.*}"
}

# Replaces file name extension.
#
# @param $1 File name ending with an extension: '.' followed by 0+ non-'.'s.
# @param $2 New extension:  '.' followed by 0+ non-'.'s.
# @return File name with the new extension.
function replace_extension() {
  echo "${1%.*}$2"
}


# Inserts given value before each element of given array, eg.
#
# arr=("a1" "a2")
# prepend_each "p" ${arr[@]}  # returns "p" "a1" "p" "a2"
# 
# @param $1 The value to insert.
# @param $2..$n The array.
# @return The array with the value inserted before each element.
function prepend_each() {
  local value_to_prepend=$1
  for arg in "${@:2}"; do
    echo $value_to_prepend
    echo $arg
  done
}
