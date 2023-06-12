
import collections
import struct  # TODO

import pandas as pd

from controller.debugging import log_message


def PrintDistances(log):
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

  while True:
    message = DistanceMessage.FromBinary(log)
    sensors[message.sensor_id].UpdateDistance(message)
    last_reading_micros = message.echo_low_micros
    print '%04u.%06u ' % divmod(last_reading_micros, 1000000),
    print '    '.join(map(str, sensors.values()))


def ReadDistances(log,
                  pivot_index_column=None, pivot_value_columns='distance_mm'):
  messages = []
  while True:
    try:
      message = DistanceMessage.FromBinary(log)
    except struct.error:
      break
    messages.append(message.args)

  df = pd.DataFrame(messages, columns=[
    'sensor_id', 'distance_mm',
    'trig_low_micros', 'echo_high_micros', 'echo_low_micros'])
  for c in ('trig_low_micros', 'echo_high_micros', 'echo_low_micros'):
    df[c] = [pd.Timedelta(microseconds=s) for s in df[c]]

  if not pivot_index_column:
    return df
  else:
    if isinstance(pivot_value_columns, str):  # Single value column.
      return df.pivot(
        index=pivot_index_column,
        columns='sensor_id',
        values=pivot_value_columns)
    else:  # Many value columns.
      return df[['sensor_id', pivot_index_column] + pivot_value_columns].pivot(
        index=pivot_index_column,
        columns='sensor_id')


class DistanceMessage(log_message.LogMessage):
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
