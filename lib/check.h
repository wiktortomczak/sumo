
#pragma once

// TODO: No-op if NDEBUG.

#ifdef __AVR__  // Building for Arduino

// Temporary dummy CHECK, needed by log.h, while DLOG has not been defined.
#define CHECK(expr)

#include "lib/log.h"

#undef CHECK
#define CHECK(expr)  \
  ((expr)  \
   ? __NO_OP_EXPRESSION  \
   : ((LOG(FATAL) << P("check failed: ") << P(#expr)), \
      __NO_OP_EXPRESSION))

#define __NO_OP_EXPRESSION static_cast<void>(0)

#else  // Building for Linux. TODO: Merge with __AVR__ branch.

#include <cassert>
#define CHECK assert

#endif
