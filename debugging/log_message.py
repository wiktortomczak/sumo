
import struct


def PrintBinaryLogMessages(log):
  while True:
    print LogMessage.FromBinary(log).ToLogLine()


class LogMessage(object):

  @classmethod
  def FromBinary(cls, log):
    message_size, = struct.unpack('H', log.read(2))
    binary = log.read(message_size)
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

LogMessage._UNPACK_VALUE_FUNCS = {
  1: LogMessage._UnpackUint8,
  2: LogMessage._UnpackUint16,
  3: LogMessage._UnpackUint32,
  4: LogMessage._UnpackString
}
