#pragma once

#include "lib/log-private.h"

#ifdef NDEBUG
#define DLOG(severity) internal::log::NullStream()
#else
#define DLOG(severity) LOG(severity)
#endif


#ifdef __AVR__  // Building for Arduino.

#include "arduino-ext/pgm.h"
#include "os/timer-global.h"
#include "os/thread.h"

enum class Severity : uint8_t;

// TODO: FATAL to LOG_UNBUFFERED.
#define LOG(severity)  \
  ((Severity::severity == Severity::FATAL) || !Thread::is_interrupt()  ? \
    LOG_OBJECT.BeginMessage(  \
      Severity::severity, timer.Now(), Thread::id, P(__FILE__), __LINE__) :  \
    LOG_OBJECT.BeginAsyncMessage(  \
      Severity::severity, timer.Now(), Thread::id, P(__FILE__), __LINE__))

// Temporary LOG_OBJECT. Some LOG is needed by log implementation itself.
#define LOG_OBJECT internal::log::NullStream()  // TODO

#include "lib/log_buffered_binary_serial.h"

#define LOG_UNBUFFERED(severity) \
  binary_serial_log.BeginMessage(  \
    Severity::severity, timer.Now(), Thread::id, P(__FILE__), __LINE__)

#undef LOG
#define LOG(severity) LOG_UNBUFFERED(severity)

// #include "lib/log_text_serial.h"

#else  // Building for Linux.

#include <iostream>

#define LOG(severity) std::cout  // TODO
#define P(str) str

#endif
