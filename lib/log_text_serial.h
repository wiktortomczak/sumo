
#pragma once

#include "arduino-ext/hardware_serial_stream.h"
#include "lib/text_log.h"

inline volatile TextLog<HardwareSerialStream> text_serial_log;

#undef LOG_OBJECT
#define LOG_OBJECT text_serial_log
