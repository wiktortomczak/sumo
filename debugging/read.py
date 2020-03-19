#!/usr/bin/env python

"""Reads lines from /dev/ttyUSB0 and prints them to stdout.

Meant to read messages from the Arduino board.
"""

# In the current form this is no better than $ cat /dev/ttyUSB0
# However, it provides a place for more elaborate processing.


import serial

s = serial.Serial('/dev/ttyUSB0')

while True:
  print s.readline(),
