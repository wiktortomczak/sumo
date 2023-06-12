// TODO: Rename to log_buffered_binary_serial.h

#pragma once

#include "arduino-ext/hardware_serial_stream.h"
#include "lib/binary_log.h"
#include "lib/buffered_log.h"

volatile inline BinaryLog<HardwareSerialStream> binary_serial_log;

volatile inline BufferedLog<decltype(binary_serial_log), 256>
  buffered_binary_serial_log;

#undef LOG_OBJECT
#define LOG_OBJECT buffered_binary_serial_log
