#!/usr/bin/env python

"""Reads lines from /dev/ttyUSB0 and prints them to stdout.

Meant to read messages from the Arduino board.
"""

import collections
import os
import struct
import sys

import serial


def main(argv):
  if len(argv) >= 2:
    read_func = _READ_FUNCS[argv[1]]
  else:
    read_func = _READ_FUNCS['-l']

  port = os.environ.get('AVR_PORT', '/dev/ttyUSB0')
  serial_port = serial.Serial(port=port, baudrate=115200)

  read_func(serial_port)


def ReadLines(serial_port):
  while True:
    print serial_port.readline(),

    
def ReadChars(serial_port):
  while True:
    print '0x%02x' % ord(serial_port.read(1))
      

def ReadBinary(serial_port):
  while True:
    print Message.FromBinary(serial_port).ToLogLine()


def ReadDistances(serial_port):
  last_reading_micros = None

  class Sensor(object):
    def __init__(self, id):
      self._id = id
      self._distance_mm = 0

    def UpdateDistance(self, distance_message):
      self._last_message = distance_message

    def __str__(self):
      if not hasattr(self, '_last_message'):
        return self._id
      
      lm = self._last_message
      reading_micros = (lm.echo_high_micros + lm.echo_low_micros) / 2
      reading_delay_ms = (last_reading_micros - reading_micros) / 1000
        
      return '%s=%4u mm  +%3ums' % (self._id, lm.distance_mm, reading_delay_ms)
    
  sensors = collections.OrderedDict([
    ('left', Sensor('left')),
    ('front', Sensor('front')),
    ('right', Sensor('right'))
  ])

  class DistanceMessage(Message):
    @property
    def sensor_id(self):
      return self.args[0]
      
    @property
    def distance_mm(self):
      return self.args[1]
      
    @property
    def trig_low_micros(self):
      return self.args[2]
      
    @property
    def echo_high_micros(self):
      return self.args[3]
      
    @property
    def echo_low_micros(self):
      return self.args[4]
  
  while True:
    message = DistanceMessage.FromBinary(serial_port)
    sensors[message.sensor_id].UpdateDistance(message)
    last_reading_micros = message.echo_low_micros
    print '%04u.%06u ' % divmod(last_reading_micros, 1000000),
    print '    '.join(map(str, sensors.values()))


_READ_FUNCS = {
  '-l': ReadLines,
  '-c': ReadChars,
  '-b': ReadBinary,
  '-d': ReadDistances
}


class Message(object):

  @classmethod
  def FromBinary(cls, source):
    message_size, = struct.unpack('H', source.read(2))
    binary = source.read(message_size)
    micros, = struct.unpack_from('I', binary)
    file_name, _ = cls._UnpackString(binary, 4)
    line_number, severity_binary, num_args = (
      struct.unpack_from('HBB', binary, 4 + 1 + len(file_name)))
    offset = 4 + 1 + len(file_name) + 2 + 1 + 1
    
    args = []
    for _ in range(num_args):
      arg, arg_size = cls._UnpackValue(binary, offset)
      args.append(arg)
      offset += arg_size

    severity = cls._SEVERITY[severity_binary]
    return cls(severity, micros, file_name, line_number, args)

  def __init__(self, severity, micros, file_name, line_number, args):
    self.severity = severity
    self.micros = micros
    self.file_name = file_name
    self.line_number = line_number
    self.args = args

  def ToLogLine(self):
    return '%(severity)s%(micros)s %(file_name)s:%(line_number)d: %(args)s' % dict(
      severity=self.severity[0],
      micros='%04d.%06d' % divmod(self.micros, 1000000),
      file_name=self.file_name,
      line_number=self.line_number,
      args=''.join(map(str, self.args)))

  _SEVERITY = {1: 'FATAL', 2: 'INFO'}
      

  @classmethod
  def _UnpackValue(cls, binary, offset):
    value_type, = struct.unpack_from('B', binary, offset)
    value, value_size = cls._UNPACK_VALUE_FUNCS[value_type](binary, offset + 1)
    return value, value_size + 1

  @classmethod
  def _UnpackUint8(cls, binary, offset):
    value, = struct.unpack_from('B', binary, offset)
    return value, 1

  @classmethod
  def _UnpackUint16(cls, binary, offset):
    value, = struct.unpack_from('H', binary, offset)
    return value, 2

  @classmethod
  def _UnpackUint32(cls, binary, offset):
    value, = struct.unpack_from('I', binary, offset)
    return value, 4

  @classmethod
  def _UnpackString(cls, binary, offset):
    size, = struct.unpack_from('B', binary, offset)
    return binary[offset+1:offset+1+size], size + 1

Message._UNPACK_VALUE_FUNCS = {
  1: Message._UnpackUint8,
  2: Message._UnpackUint16,
  3: Message._UnpackUint32,
  4: Message._UnpackString
}

    
# Debugging helpers.

class SerialDebugProxy(object):

  def __init__(self, s):
    self._s = s

  def read(self, size):
    binary = self._s.read(size)
    print StringToHex(binary)
    return binary


def StringToHex(s):
  return ' '.join(['0x%02x' % ord(c) for c in s])


if __name__ == '__main__':
  main(sys.argv)
