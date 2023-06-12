#!/usr/bin/env python

"""Reads lines from /dev/ttyUSB0 and prints them to stdout.

Meant to read messages from the Arduino board.
"""

import sys

import gflags
import serial

import log_message
from distance_sensors import distance_log

gflags.DEFINE_string('format', 'lines', '')
gflags.DEFINE_string('serial', '/dev/ttyUSB0', '')
gflags.DEFINE_string('file', None, '')
FLAGS = gflags.FLAGS


def main():
  gflags.FLAGS(sys.argv)
  
  print_func = _PRINT_FUNCS[FLAGS.format]
  if not FLAGS.file:
    log = serial.Serial(port=FLAGS.serial, baudrate=115200)
  else:
    log = file(FLAGS.file, 'r')

  print_func(log)


def PrintLines(log):
  while True:
    print log.readline(),

    
def PrintChars(log):
  while True:
    print '0x%02x' % ord(log.read(1))


_PRINT_FUNCS = {
  'lines': PrintLines,
  'chars': PrintChars,
  'binary': log_message.PrintBinaryLogMessages,
  'distances': distance_log.PrintDistances
}


# Debugging helpers.

class FileProxy(object):

  def __init__(self, f):
    self._f = f

  def read(self, size):
    binary = self._f.read(size)
    print StringToHex(binary)
    return binary


def StringToHex(s):
  return ' '.join(['0x%02x' % ord(c) for c in s])


if __name__ == '__main__':
  main()
